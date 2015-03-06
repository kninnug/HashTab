/*
 **************************************************************************
 *                                                                        *
 *          General Purpose Hash Function Algorithms Library              *
 *                                                                        *
 * Author: Arash Partow - 2002                                            *
 * URL: http://www.partow.net                                             *
 * URL: http://www.partow.net/programming/hashfunctions/index.html        *
 *                                                                        *
 * Copyright notice:                                                      *
 * Free use of the General Purpose Hash Function Algorithms Library is    *
 * permitted under the guidelines and in accordance with the most current *
 * version of the Common Public License.                                  *
 * http://www.opensource.org/licenses/cpl1.0.php                          *
 *                                                                        *
 **************************************************************************
*/

#ifndef INCLUDE_GENERALHASHFUNCTION_C_H
#define INCLUDE_GENERALHASHFUNCTION_C_H

#include <stdlib.h>

unsigned int RSHash  (const char * str, size_t len);
unsigned int JSHash  (const char * str, size_t len);
unsigned int PJWHash (const char * str, size_t len);
unsigned int ELFHash (const char * str, size_t len);
unsigned int BKDRHash(const char * str, size_t len);
unsigned int SDBMHash(const char * str, size_t len);
unsigned int DJBHash (const char * str, size_t len);
unsigned int DEKHash (const char * str, size_t len);
unsigned int BPHash  (const char * str, size_t len);
unsigned int FNVHash (const char * str, size_t len);
unsigned int APHash  (const char * str, size_t len);

#endif
