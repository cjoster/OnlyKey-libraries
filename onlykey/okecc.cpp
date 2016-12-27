
/* Tim Steiner
 * Copyright (c) 2016 , CryptoTrust LLC.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *      
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OnlyKey Project
 *    (http://www.crp.to/ok)"
 *
 * 4. The names "OnlyKey" and "OnlyKey Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    admin@crp.to.
 *
 * 5. Products derived from this software may not be called "OnlyKey"
 *    nor may "OnlyKey" appear in their names without prior written
 *    permission of the OnlyKey Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OnlyKey Project
 *    (http://www.crp.to/ok)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OnlyKey PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OnlyKey PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * Thanks to Thomas Sileo for making SSH support possible
 */



#include "okecc.h"
#include <SoftTimer.h>
#include <cstring>
#include "Arduino.h"
#include "onlykey.h"

#ifdef US_VERSION

/*************************************/
//ECC Authentication assignments
/*************************************/
const char ecc_stored_private_key[] = "\xF4\x2C\x74\xF8\x03\x50\xD0\x05\xEA\x82\x80\x1C\x95\xD2\x82\xCB\xB8\x1E\x6E\xF3\x63\xF7\x67\x59\xE8\x14\x0F\xBF\x31\x4D\x68\xA0";
uint8_t ecc_signature[64];
uint8_t ecc_public_key[32];
uint8_t ecc_private_key[32];
uint8_t ECC_button = 0;
uint8_t ECC_AUTH = 0;
/*************************************/

void GETECCPUBKEY (uint8_t *buffer)
{
		onlykey_flashget_ECC (buffer[5]);        
			#ifdef DEBUG
    	    Serial.println("OKGETECCPUBKEY MESSAGE RECEIVED"); 
	    for (int i = 0; i< 32; i++) {
    	    Serial.print(ecc_public_key[i],HEX);
     	    }
	    #endif
            RawHID.send(ecc_public_key, 32);
			memset(ecc_public_key, 0, 32); //wipe buffer
			memset(ecc_private_key, 0, 32); //wipe buffer
            blink(3);
}

void SIGNECCCHALLENGE (uint8_t *buffer)
{
#ifdef DEBUG
    Serial.println();
    Serial.println("OKSIGNECCCHALLENGE MESSAGE RECEIVED"); 
#endif
	ECC_AUTH = 1;
    if(ECC_button) {
    // XXX(tsileo): on my system the challenge always seems to be 147 bytes, but I keep it dynamic
    // // since it may change.
	extern int large_data_offset;
	extern uint8_t large_buffer[1024];
    if (buffer[6]==0xFF) //Not last packet
    {
        // TODO(tsileo): best max size
        if (large_data_offset <= 768) {
            memcpy(large_buffer+large_data_offset, buffer+7, 57);
            large_data_offset = large_data_offset + 57;
        } else {
              hidprint("Error ECC challenge larger than 768 bytes");
        }
        return;
    } else {
        if (large_data_offset <= 710 && buffer[6] <= 57) {
            memcpy(large_buffer+large_data_offset, buffer+7, buffer[6]);
            large_data_offset = large_data_offset + buffer[6];
        } else {
            hidprint("Error ECC challenge larger than 768 bytes");
        }
    }

#ifdef DEBUG
    Serial.println();
    Serial.printf("ECC challenge blob size=%d", large_data_offset);
#endif

	int type = onlykey_flashget_ECC (buffer[5]); 
	const struct uECC_Curve_t * curves[2];
    int num_curves = 0;
    curves[num_curves++] = uECC_secp256r1();
    curves[num_curves++] = uECC_secp256k1();
	// Sign the blob stored in the buffer
	if (type==0x01) Ed25519::sign(ecc_signature, ecc_private_key, ecc_public_key, large_buffer, large_data_offset);
	else if (type==0x02) {
		uECC_sign(ecc_private_key, large_buffer, large_data_offset, ecc_signature, curves[1]);
	}
	else if (type==0x03) {
		uECC_sign(ecc_private_key, large_buffer, large_data_offset, ecc_signature, curves[2]);
	}
    
    // Reset the large buffer offset
    large_data_offset = 0;
	memset(large_buffer, 0, 1024); //wipe buffer
    // Stop the fade in
    fadeoff();

    // Send the signature
    /* hidprint((const char*)ecc_signature); */
#ifdef DEBUG
	    for (int i = 0; i< 64; i++) {
    	    Serial.print(ecc_signature[i],HEX);
     	    }
#endif
    RawHID.send(ecc_signature, 64);
	ECC_AUTH = 0;
	ECC_button = 0;
    blink(3);
	}
	
	memset(ecc_signature, 0, 64); //wipe buffer
	memset(ecc_public_key, 0, 32); //wipe buffer
	memset(ecc_private_key, 0, 32); //wipe buffer
    return;
}

#endif