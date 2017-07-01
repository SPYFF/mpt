/**
 * @file
 * @author Kelemen Tamas <kiskele@krc.hu>
 * @brief Macro and function definitons for auth.c
 */

#ifndef _AUTH_H_
#define _AUTH_H_

/**
 * Simple macro to convert a HEX digit (0-9a-fA-F) into decimal value
 */
#define HEX2DEC(x) (x>='0' && x<='9' ? x-'0' : (x>='a' && x<='f' ? x-'a'+10 : (x>='A' && x<='F' ? x-'A'+10 : 0)))

int sha256(char * buff, int size, char * key, int keysize, char * out, int * outsize);
void printHash(char * buff, int size);
int authSize(int authType);
int authTest(connection_type * conn, char * buff, int size);
void authSet(connection_type * conn, char * buff, int size);

void setKey(connection_type * conn, char * hex);
void saveKey(FILE * fd, connection_type * conn);
int keySize(int authType);

#endif
