/**
 * Multiparty Off-the-Record Messaging library
 * Copyright (C) 2014, eQualit.ie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef SRC_CRYPT_H_
#define SRC_CRYPT_H_

#include <string>
#include <cstring>

#include "src/common.h"
#include "src/exceptions.h"
#include "src/crypt.h"
#include "common.h"
#include "exceptions.h"

extern "C" {
#include "gcrypt.h"
}

namespace np1sec
{

typedef gcry_sexp_t AsymmetricKey;

typedef HashBlock np1secKeyShare;
typedef HashBlock np1secSymmetricKey;

const unsigned int c_ephemeral_key_length = 32;
const unsigned int c_key_share = c_hash_length;
const unsigned int c_iv_length = 16;

typedef uint8_t IVBlock[c_iv_length];

std::string public_key_to_stringbuff(AsymmetricKey public_key);

/**
 * Generates a random ed25519 key pair
 * @return false in case of error otherwise true
 */
bool generate_key_pair(AsymmetricKey* generated_key);

/**
 *  get a key pair generated by libgcypt in sexp_t format
 *  and extract out the public key (for announcement)
 *  key need to be released
 */
gcry_sexp_t get_public_key(AsymmetricKey key_pair);

/**
 * Convert a given gcrypt s-expression into a std::string
 * @param gcry_sexp_t gcrypt s-expression to be converted
 * @return std::string representing the converted data.
 */
std::string retrieve_result(gcry_sexp_t text_sexp);

void release_crypto_resource(gcry_sexp_t crypto_resource);

gcry_sexp_t copy_crypto_resource(gcry_sexp_t crypto_resource);

gcry_error_t hash(const void* buffer, size_t buffer_len, HashBlock hb);
gcry_error_t hash(const void* buffer, size_t buffer_len, HashBlock hb, bool secure);

gcry_error_t hash(const std::string string_buffer, HashBlock hb);
gcry_error_t hash(const std::string string_buffer, HashBlock hb, bool secure);

HashStdBlock hash(const std::string string_buffer);
HashStdBlock hash(const std::string string_buffer, bool secure);

int compare_hash(const HashBlock rhs, const HashBlock lhs);

std::string hash_to_string_buff(const HashBlock hash_block);

const uint8_t* strbuff_to_hash(std::string& hash_block_buffer);

PublicKey extract_public_key(const AsymmetricKey complete_key);

/**
 * This function gets the value for q section of public-key and
 * reconstruct the whole sexp to be used in libgcrypt functions
 */
AsymmetricKey reconstruct_public_key_sexp(const std::string pub_key_block);
    
/**
 * Convert a given std:string to a valid gcrypt s-expression
 * @param std::string valid string to be converted
 * @return gcry_sexp_t gcrypt s-expression respresentation
 */
gcry_sexp_t convert_to_sexp(std::string text);

/**
 * Encryption primitives and related definitions.
 */
class Cryptic
{
  protected:
    gcry_sexp_t ephemeral_key = nullptr;
    gcry_sexp_t ephemeral_pub_key = nullptr;
    gcry_sexp_t ephemeral_prv_key = nullptr;
    np1secSymmetricKey session_key;

    // HashBlock session_iv; //TODO:: it might be good to have a iv for the whole
    // session

    static const uint32_t ED25519_KEY_SIZE = 32;
    static const gcry_mpi_format NP1SEC_BLOB_OUT_FORMAT = GCRYMPI_FMT_USG;

  public:
    void set_session_key(const np1secSymmetricKey session_key)
    {
        memcpy(this->session_key, session_key, sizeof(np1secSymmetricKey));
    }

    /**
     * Constructor setup the key
     */
    Cryptic();

    /**
     * Copy constructor
     */
    Cryptic(const Cryptic& rhs)
    {
        ephemeral_key = copy_crypto_resource(rhs.ephemeral_key);
        ephemeral_pub_key = copy_crypto_resource(rhs.ephemeral_pub_key);
        ephemeral_prv_key = copy_crypto_resource(rhs.ephemeral_prv_key);
        set_session_key(rhs.session_key);
    }

    /**
     * Access function for ephemeral pub key
     * (Access is need for meta works like computing the session id  which are
     *  not crypto task per se)
     *
     */
    gcry_sexp_t get_ephemeral_pub_key() const { return ephemeral_pub_key; }


    bool init();

    /**
     * Encrypt a give plain text using the previously created ed25519 keys
     * @param plain_text a plain text message string to be encrypted
     * @return a string containing the encrypted text
     */
    std::string Encrypt(std::string plain_text);

    /**
     * Decrypt a give encrypted text using the previously created ed25519 keys teddh
     * @param encrypted_text an encrypted text message string to be decrypted
     * @return a string containing the decrypted text
     */
    std::string Decrypt(std::string encrypted_text);

    /**
     * Given the peer's long term and ephemeral public key AP and ap, and ours
     * BP, bP, all points on ed25519 curve, this
     * compute the triple dh value.
     *n
     * @param peer_ephemeral_key the ephemeral public key of peer i.e. aP
     *                           in grcypt eddsa public key format
     * @param peer_long_term_key the long term public key of the peer i.e AP
     *                            in gcrypt eddsa public key format
     * @param my_long_term_key   our longterm key in eddsa format
     * @param peer_is_first      true if AP.X|AP.Y < BP.X|BP.Y
     * @param teddh_token        a pointer to hash block to store
     *        hash(bAP|BaP|baP) if peer_is_first
     *        hash(BaP|bAP|baP) in GCRYMPI_FMT_USG format if the pointer is null
     *         , necessary space will be allocated.
     *
     * throw an exception  if the operation fails, true on success
     */
    void triple_ed_dh(PublicKey peer_ephemeral_key, PublicKey peer_long_term_key,
                      AsymmetricKey my_long_term_key, bool peer_is_first, Token* teddh_token);

    /**
     * Given a valid std:string sign the string using the sessions
     * private key and store the signature in *sigp with *siglenp
     * throw exception in case of failure
     *
     * @param unsigned char ** representing a buffer to store the create signature
     * @param size_t representing the length of the return sig buffer
     * @parama std::string representing the message to be signed
     *
     *
     */
    void sign(unsigned char** sigp, size_t* siglenp, std::string plain_text);

    /**
     * Given a signed piece of data and a valid signature verify if
     * the signature is correct using the sessions public key.
     *
     * @param std::string representing signed data
     * @param const unsigned char*  representing data signature buffer
     * @param the public key of the party who has signed the message
     *
     * @return true if the signature is valid false on false signature
     * throw exception in case of error
     */
    bool verify(std::string signed_text, const unsigned char* sigbuf, PublicKey signer_ephmeral_pub_key);

    /**
     * Create instance of cipher session based on configured algorithm, mode,
     * key and iv.
     *
     * @return gcry_cipher_hd_t representing a cipher session handle
     */
    gcry_cipher_hd_t OpenCipher();

    /**
     * Zeroising all secret key materials
     *
     */
    ~Cryptic();
};

class LongTermIDKey
{
  protected:
    KeyPair key_pair;
    bool initiated = false;

  public:
    /**
     * constructor
     */
    LongTermIDKey() : key_pair(nullptr, nullptr) {}

    /**
     * destructor
     */
    ~LongTermIDKey()
    {
        release_crypto_resource(key_pair.first);
        release_crypto_resource(key_pair.second);
    }

    /**
     * Access
     */
    int is_initiated() { return initiated; }

    KeyPair get_key_pair() { return key_pair; }

    PublicKey get_public_key() const { return key_pair.second; }

    PublicKey get_private_key() const { return key_pair.first; }

    /**
     * Initiation
     */
    /**
     * @return false if key generation goes wrong (for example due to
     *         lack of entropy
     */
    void generate()
    {
        try {
            generate_key_pair(&key_pair.first);
            key_pair.second = extract_public_key(key_pair.first);
            initiated = true;
        } catch (CryptoException& crypto_exception) {
            // rethrow
            throw crypto_exception;
        }
    }

    void set_key_pair(KeyPair user_key_pair)
    {
        initiated = true;
        key_pair.first = copy_crypto_resource(user_key_pair.first);
        key_pair.second = copy_crypto_resource(user_key_pair.second);
    }

    /**
     * treat the raw_key_pair as gcrypt_sexp turned
     * int string buffer
     *
     * @param raw_key_pair is not a pair but the sexp converted
     *        string of the ed25519 key
     */
    void set_key_pair(uint8_t* raw_key_pair)
    {
        try {
            key_pair.first = convert_to_sexp(hash_to_string_buff(raw_key_pair));
            // because we never transmit the
            // private key, it doesn't matter what is the thingi that we
            // call private key.
            key_pair.second = extract_public_key(key_pair.first);

        } catch (CryptoException& crypto_exception) {
            throw crypto_exception; // the raw key is given by the client and we
            // should tell them that their key is corrupted
        }

        initiated = true;
    }
};

const int c_np1sec_hash = gcry_md_algos::GCRY_MD_SHA256;

} // namespace np1sec

#endif // SRC_CRYPT_H_
