// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/webcrypto/status.h"

#include "base/format_macros.h"
#include "base/strings/stringprintf.h"

namespace content {

// TODO(eroman): The error text for JWK uses the terminology "property" however
// it should instead call it a "member". Changing this needs to coordinate with
// the Blink LayoutTests as they depend on the old names.

namespace webcrypto {

bool Status::IsError() const {
  return type_ == TYPE_ERROR;
}

bool Status::IsSuccess() const {
  return type_ == TYPE_SUCCESS;
}

Status Status::Success() {
  return Status(TYPE_SUCCESS);
}

Status Status::OperationError() {
  return Status(blink::WebCryptoErrorTypeOperation, "");
}

Status Status::DataError() {
  return Status(blink::WebCryptoErrorTypeData, "");
}

Status Status::ErrorJwkNotDictionary() {
  return Status(blink::WebCryptoErrorTypeData,
                "JWK input could not be parsed to a JSON dictionary");
}

Status Status::ErrorJwkMemberMissing(const std::string& member_name) {
  return Status(
      blink::WebCryptoErrorTypeData,
      "The required JWK property \"" + member_name + "\" was missing");
}

Status Status::ErrorJwkMemberWrongType(const std::string& member_name,
                                       const std::string& expected_type) {
  return Status(
      blink::WebCryptoErrorTypeData,
      "The JWK property \"" + member_name + "\" must be a " + expected_type);
}

Status Status::ErrorJwkBase64Decode(const std::string& member_name) {
  return Status(
      blink::WebCryptoErrorTypeData,
      "The JWK property \"" + member_name + "\" could not be base64 decoded");
}

Status Status::ErrorJwkExtInconsistent() {
  return Status(
      blink::WebCryptoErrorTypeData,
      "The \"ext\" property of the JWK dictionary is inconsistent what that "
      "specified by the Web Crypto call");
}

Status Status::ErrorJwkAlgorithmInconsistent() {
  return Status(blink::WebCryptoErrorTypeData,
                "The JWK \"alg\" property was inconsistent with that specified "
                "by the Web Crypto call");
}

Status Status::ErrorJwkUnrecognizedUse() {
  return Status(blink::WebCryptoErrorTypeData,
                "The JWK \"use\" property could not be parsed");
}

Status Status::ErrorJwkUnrecognizedKeyop() {
  return Status(blink::WebCryptoErrorTypeData,
                "The JWK \"key_ops\" property could not be parsed");
}

Status Status::ErrorJwkUseInconsistent() {
  return Status(blink::WebCryptoErrorTypeData,
                "The JWK \"use\" property was inconsistent with that specified "
                "by the Web Crypto call. The JWK usage must be a superset of "
                "those requested");
}

Status Status::ErrorJwkKeyopsInconsistent() {
  return Status(blink::WebCryptoErrorTypeData,
                "The JWK \"key_ops\" property was inconsistent with that "
                "specified by the Web Crypto call. The JWK usage must be a "
                "superset of those requested");
}

Status Status::ErrorJwkUseAndKeyopsInconsistent() {
  return Status(blink::WebCryptoErrorTypeData,
                "The JWK \"use\" and \"key_ops\" properties were both found "
                "but are inconsistent with each other.");
}

Status Status::ErrorJwkUnexpectedKty(const std::string& expected) {
  return Status(blink::WebCryptoErrorTypeData,
                "The JWK \"kty\" property was not \"" + expected + "\"");
}

Status Status::ErrorJwkIncorrectKeyLength() {
  return Status(blink::WebCryptoErrorTypeData,
                "The JWK \"k\" property did not include the right length "
                "of key data for the given algorithm.");
}

Status Status::ErrorJwkEmptyBigInteger(const std::string& member_name) {
  return Status(blink::WebCryptoErrorTypeData,
                "The JWK \"" + member_name + "\" property was empty.");
}

Status Status::ErrorJwkBigIntegerHasLeadingZero(
    const std::string& member_name) {
  return Status(
      blink::WebCryptoErrorTypeData,
      "The JWK \"" + member_name + "\" property contained a leading zero.");
}

Status Status::ErrorJwkDuplicateKeyOps() {
  return Status(blink::WebCryptoErrorTypeData,
                "The \"key_ops\" property of the JWK dictionary contains "
                "duplicate usages.");
}

Status Status::ErrorImportEmptyKeyData() {
  return Status(blink::WebCryptoErrorTypeData, "No key data was provided");
}

Status Status::ErrorUnsupportedImportKeyFormat() {
  return Status(blink::WebCryptoErrorTypeNotSupported,
                "Unsupported import key format for algorithm");
}

Status Status::ErrorUnsupportedExportKeyFormat() {
  return Status(blink::WebCryptoErrorTypeNotSupported,
                "Unsupported export key format for algorithm");
}

Status Status::ErrorImportAesKeyLength() {
  return Status(blink::WebCryptoErrorTypeData,
                "AES key data must be 128, 192 or 256 bits");
}

Status Status::ErrorAes192BitUnsupported() {
  return Status(blink::WebCryptoErrorTypeNotSupported,
                "192-bit AES keys are not supported");
}

Status Status::ErrorUnexpectedKeyType() {
  return Status(blink::WebCryptoErrorTypeInvalidAccess,
                "The key is not of the expected type");
}

Status Status::ErrorIncorrectSizeAesCbcIv() {
  return Status(blink::WebCryptoErrorTypeData,
                "The \"iv\" has an unexpected length -- must be 16 bytes");
}

Status Status::ErrorIncorrectSizeAesCtrCounter() {
  return Status(blink::WebCryptoErrorTypeData,
                "The \"counter\" has an unexpected length -- must be 16 bytes");
}

Status Status::ErrorInvalidAesCtrCounterLength() {
  return Status(blink::WebCryptoErrorTypeData,
                "The \"length\" property must be >= 1 and <= 128");
}

Status Status::ErrorAesCtrInputTooLongCounterRepeated() {
  return Status(blink::WebCryptoErrorTypeData,
                "The input is too large for the counter length.");
}

Status Status::ErrorDataTooLarge() {
  return Status(blink::WebCryptoErrorTypeData,
                "The provided data is too large");
}

Status Status::ErrorDataTooSmall() {
  return Status(blink::WebCryptoErrorTypeData,
                "The provided data is too small");
}

Status Status::ErrorUnsupported() {
  return ErrorUnsupported("The requested operation is unsupported");
}

Status Status::ErrorUnsupported(const std::string& message) {
  return Status(blink::WebCryptoErrorTypeNotSupported, message);
}

Status Status::ErrorUnexpected() {
  return Status(blink::WebCryptoErrorTypeUnknown,
                "Something unexpected happened...");
}

Status Status::ErrorInvalidAesGcmTagLength() {
  return Status(
      blink::WebCryptoErrorTypeData,
      "The tag length is invalid: Must be 32, 64, 96, 104, 112, 120, or 128 "
      "bits");
}

Status Status::ErrorInvalidAesKwDataLength() {
  return Status(blink::WebCryptoErrorTypeData,
                "The AES-KW input data length is invalid: not a multiple of 8 "
                "bytes");
}

Status Status::ErrorGenerateKeyPublicExponent() {
  return Status(blink::WebCryptoErrorTypeData,
                "The \"publicExponent\" must be either 3 or 65537");
}

Status Status::ErrorImportRsaEmptyModulus() {
  return Status(blink::WebCryptoErrorTypeData, "The modulus is empty");
}

Status Status::ErrorGenerateRsaUnsupportedModulus() {
  return Status(blink::WebCryptoErrorTypeNotSupported,
                "The modulus length must be a multiple of 8 bits and >= 256 "
                "and <= 16384");
}

Status Status::ErrorImportRsaEmptyExponent() {
  return Status(blink::WebCryptoErrorTypeData,
                "No bytes for the exponent were provided");
}

Status Status::ErrorKeyNotExtractable() {
  return Status(blink::WebCryptoErrorTypeInvalidAccess,
                "They key is not extractable");
}

Status Status::ErrorGenerateKeyLength() {
  return Status(blink::WebCryptoErrorTypeData,
                "Invalid key length: it is either zero or not a multiple of 8 "
                "bits");
}

Status Status::ErrorCreateKeyBadUsages() {
  return Status(blink::WebCryptoErrorTypeSyntax,
                "Cannot create a key using the specified key usages.");
}

Status Status::ErrorImportedEcKeyIncorrectCurve() {
  return Status(
      blink::WebCryptoErrorTypeData,
      "The imported EC key specifies a different curve than requested");
}

Status Status::ErrorJwkIncorrectCrv() {
  return Status(
      blink::WebCryptoErrorTypeData,
      "The JWK's \"crv\" member specifies a different curve than requested");
}

Status Status::ErrorEcKeyInvalid() {
  return Status(blink::WebCryptoErrorTypeData,
                "The imported EC key is invalid");
}

Status Status::JwkOctetStringWrongLength(const std::string& member_name,
                                         size_t expected_length,
                                         size_t actual_length) {
  return Status(
      blink::WebCryptoErrorTypeData,
      base::StringPrintf(
          "The JWK's \"%s\" member defines an octet string of length %" PRIuS
          " bytes but should be %" PRIuS,
          member_name.c_str(), actual_length, expected_length));
}

Status::Status(blink::WebCryptoErrorType error_type,
               const std::string& error_details_utf8)
    : type_(TYPE_ERROR),
      error_type_(error_type),
      error_details_(error_details_utf8) {
}

Status::Status(Type type) : type_(type) {
}

}  // namespace webcrypto

}  // namespace content
