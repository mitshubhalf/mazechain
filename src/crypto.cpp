#include "../include/crypto.h"
#include <openssl/sha.h>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/ecdsa.h>
#include <sstream>
#include <iomanip>

std::string sha256(const std::string &data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)data.c_str(), data.size(), hash);

    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];

    return ss.str();
}

std::string generatePrivateKey() {
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    EC_KEY_generate_key(key);

    BIO* bio = BIO_new(BIO_s_mem());
    PEM_write_bio_ECPrivateKey(bio, key, NULL, NULL, 0, NULL, NULL);

    char* data;
    long len = BIO_get_mem_data(bio, &data);
    std::string privateKey(data, len);

    BIO_free(bio);
    EC_KEY_free(key);

    return privateKey;
}

std::string getPublicKey(const std::string &privateKey) {
    BIO* bio = BIO_new_mem_buf(privateKey.c_str(), -1);
    EC_KEY* key = PEM_read_bio_ECPrivateKey(bio, NULL, NULL, NULL);

    BIO* pub = BIO_new(BIO_s_mem());
    PEM_write_bio_EC_PUBKEY(pub, key);

    char* data;
    long len = BIO_get_mem_data(pub, &data);
    std::string publicKey(data, len);

    BIO_free(pub);
    BIO_free(bio);
    EC_KEY_free(key);

    return publicKey;
}

std::string signData(const std::string &data, const std::string &privateKey) {
    BIO* bio = BIO_new_mem_buf(privateKey.c_str(), -1);
    EC_KEY* key = PEM_read_bio_ECPrivateKey(bio, NULL, NULL, NULL);

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)data.c_str(), data.size(), hash);

    unsigned int sig_len;
    unsigned char* sig = new unsigned char[ECDSA_size(key)];

    ECDSA_sign(0, hash, SHA256_DIGEST_LENGTH, sig, &sig_len, key);

    std::string signature((char*)sig, sig_len);

    delete[] sig;
    EC_KEY_free(key);
    BIO_free(bio);

    return signature;
}

bool verifySignature(const std::string &data, const std::string &signature, const std::string &publicKey) {
    BIO* bio = BIO_new_mem_buf(publicKey.c_str(), -1);
    EC_KEY* key = PEM_read_bio_EC_PUBKEY(bio, NULL, NULL, NULL);

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)data.c_str(), data.size(), hash);

    int result = ECDSA_verify(0,
        hash,
        SHA256_DIGEST_LENGTH,
        (unsigned char*)signature.c_str(),
        signature.size(),
        key
    );

    EC_KEY_free(key);
    BIO_free(bio);

    return result == 1;
}
