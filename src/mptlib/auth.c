/**
 * @file
 * @author Kelemen Tamas <kiskele@krc.hu>
 * @brief Authentication and encryption functions
 */

//#define MPT_DEBUG

#include <openssl/evp.h>

#include "multipath.h"
#include "mp_local.h"
#include "auth.h"

// setglobaékey set the flobal authentication key DEFAULT_KEY or the conf.template auth_key value
void setglobalkey(void)
{
    connection_type ctemplate;
    strcpy(globalkey, DEFAULT_KEY);

    FILE *fin = fopen("conf/connections/conf.template", "r");
    if(fin) {
        strcpy(ctemplate.filename, "conf.template");
        fclose(fin);
        conn_parser(&ctemplate);
        strcpy(globalkey, ctemplate.auth_key);
    }
}

// getkey - gets the kay of the connection, or globalkey if conn == NULL
char *getkey(connection_type *conn)
{
    if (!conn) return globalkey;
    return conn->auth_key;
}

/**
 * Generate SHA-256 HASH
 *
 * @param[in]  buff          Generate hash from this string
 * @param[in]  size          Size of the buff parameter
 * @param[in]  key           Append this secret key to buff if larger then 0 byte
 * @param[in]  keysize       Size of the key parameter
 * @param[out] out           Put the generated hash key to this value
 * @param[out] outsize       Length of the generated hash
 *
 * @return This function always return true
 * @todo Do some error checking
 */
int sha256(char *buff, int size, char * key, int keysize, char * out, int * outsize) {
    EVP_MD_CTX * mdctx = NULL;
    const EVP_MD * md;
DEBUG("sha256 started\n");
    OpenSSL_add_all_digests();
    md = EVP_get_digestbyname("sha256");
    mdctx = EVP_MD_CTX_create();
    EVP_DigestInit(mdctx, md);
    EVP_DigestUpdate(mdctx, buff, size);
    if(keysize) EVP_DigestUpdate(mdctx, key, keysize);
    EVP_DigestFinal(mdctx, (unsigned char *)out, (unsigned int *)outsize);

    EVP_MD_CTX_destroy(mdctx);
    EVP_cleanup();
    return 0;
}


/**
 * Display a HASH value in HEX format on the standard output (useful for testing)
 *
 * @param buff     HASH value
 * @param size     Size of the buff parameter
 */
void printHash(char * buff, int size) {
    int i;

    for(i=0; i<size; i++) printf("%02X", (unsigned char)buff[i]);
    printf("\n");
}

/**
 * Returns how large authentication header used in the packets for a specified auth. type
 *
 * @param authType      Used authentication type. See @ref AuthTypes for the complete list
 * @return              The authentication header length
 */
int authSize(int authType) {
    switch(authType) {
        case AUTH_NONE:   return 0;
        case AUTH_RAND:   return 4;
        case AUTH_SHA256: return 36;    // authSize(AUTH_RAND) + keySize(AUTH_SHA256)

        default:          return 0;
    }
}


/**
 * Check the received packet whether the authentication data is correct
 *
 * @param conn  The connection to the actual peer
 * @param buff  Received data
 * @param size  Size of the data
 * @return False if the packet data
 *
 * @todo We could search for the connection if conn is NULL
 */
int authTest(connection_type * conn, char * buff, int size) {

    int check_waitrand = 0;
    bit_8 authtype = buff[2];
    char * authkey = getkey(conn);

    if (conn) {
        check_waitrand = 1;
        authtype = conn->auth_type;
    }
    if ((unsigned)buff[0] == CMD_CONNECTION_CREATE) authtype = AUTH_SHA256;

    int sh = authSize(authtype);
    switch(authtype) {
        case AUTH_NONE:
            break;
        case AUTH_SHA256:
        case AUTH_RAND:
            if (check_waitrand) {
                if(buff[sh+4] == 2) {
                    // save key when first response received
                    memcpy(&conn->waitrand, &buff[4], authSize(AUTH_RAND));
                } else if ((buff[sh+4] > 1) && (memcmp(&conn->waitrand, &buff[4], authSize(AUTH_RAND)) != 0) ) {
                // auth mismatch
                    DEBUG("Authentication test failed - check_waitrand. type:%d \n", authtype);
                    return 0;
                }
            }
            break;
        default:
            return 1;
    }

    if(authtype == AUTH_SHA256) {
        char key[32];
        int keysize = 32;

        // save key
        memcpy(key, &buff[8], 32);

        // clear memory
        memset(&buff[8], 0, 32);

DEBUG("Authkey test: %s\n", authkey);
        sha256(buff, size, authkey, keySize(authtype), &buff[8], &keysize);

        // check hash
        if(memcmp(key, &buff[8], 32)) {
            // restore key
            memcpy(&buff[8], key, 32);

            // return with incorrect key error
            fprintf(stderr, "Authentication test failed - sha256_memcmp type:%d \n", authtype);
            return 0;
        }
    }

DEBUG("Authentication test successfully finished\n");
    return 1;
}


/**
 * Sets the correct authentication data in the packet before sending it
 *
 * @param conn  The connection to the actual peer
 * @param buff  The packet to send
 * @param size  Size of the packet
 *
 * @todo We could search for the connection if conn is NULL
 */
void authSet(connection_type * conn, char * buff, int size) {

    int check_waitrand = 0;
    bit_8 authtype = buff[2];
    char * authkey = getkey(conn);

    if (conn) {
        check_waitrand = 1;
        authtype = conn->auth_type;
    }
    if ((unsigned char)buff[0] == CMD_CONNECTION_CREATE) {
        authtype = AUTH_SHA256;
    }
    int sh = authSize(authtype);
DEBUG("authSet started\n");
    switch(authtype) {
        case AUTH_SHA256:
        case AUTH_RAND:
            if(buff[sh+4] == 1) {
                // Initialize waitrand
                if (check_waitrand) memset(&conn->waitrand, 0, authSize(AUTH_RAND));
            } else if(buff[sh+4] == 2) { // incremented round number
                // Generate random number
                bit_32 random;
                srand(time(NULL));
                random = rand();
                if (check_waitrand) memcpy(&conn->waitrand, &random, authSize(AUTH_RAND));
            }

            // set random number in packet
            if (check_waitrand) memcpy(&buff[4], &conn->waitrand, authSize(AUTH_RAND));

            break;
        default:
            memset(&buff[4], 0, sh);
    }

    if(authtype == AUTH_SHA256) {
        int hashsize = 32;
        memset(&buff[8], 0, 32);
    DEBUG("Authkey set: %s\n", authkey);
        sha256(buff, size, authkey, keySize(authtype), &buff[8], &hashsize);
        DEBUG("sha256 authentication data set\n");
    }
}

/**
 * Converts authentication key from HEX to binary and saves it in the connection struct.
 * @param conn  The connection to use
 * @param hex   The authentication key stored in HEX format
 * @pre The auth_type needed to be set before using this function
 */
//void setKey(connection_type * conn, char * hex) {
//    int i;
//    for(i=0; i<keySize(conn->auth_type); i++) {
//        conn->auth_key[i] = HEX2DEC(hex[2*i]) << 4 | HEX2DEC(hex[2*i+1]);
//    }
//}

/**
 * Save authentication key in HEX format to the configuration file
 *
 * @param fd    The FILE pointer to the configuration file
 * @param conn  The connection which contains the authentication datas
 */
void saveKey(FILE * fd, connection_type * conn) {
    int i;

    switch(conn->auth_type) {
        case AUTH_SHA256:
            for(i=0; i<keySize(conn->auth_type); i++) {
                fprintf(fd, "%02X", (unsigned char)conn->auth_key[i]);
            }
            break;
        default:
            fprintf(fd, "0");
    }
    fprintf(fd, "\t# AUTH. KEY\n");
}

/**
 * Returns how large unique key is used for the authentication
 * @return Size of the auth_key
 */
int keySize(int authType) {
    switch(authType) {
        case AUTH_NONE:   return 0;
        case AUTH_RAND:   return 0;
        case AUTH_SHA256: return 32;

        default:          return 0;
    }
}
