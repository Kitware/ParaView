/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component for library authentication functionality.

#ifndef MI_NEURAYLIB_ILIBRARY_AUTHENTICATION_H
#define MI_NEURAYLIB_ILIBRARY_AUTHENTICATION_H

#include <mi/base/handle.h>
#include <mi/base/interface_declare.h>
#include <mi/neuraylib/ineuray.h>

#ifdef MI_PLATFORM_WINDOWS
#include <mi/base/miwindows.h>
#else
#include <cstdlib>
#include <ctime>
#include <sys/time.h>
#endif

#include <cstring>

namespace mi
{

class IString;

namespace neuraylib
{

/** \addtogroup mi_neuray_configuration
@{
*/

/// This interface is used for authenticating the application against the library.
///
/// Different variants of the \neurayLibraryName use different mechanisms to prevent unauthorized
/// use of the library. Two such variants are SPM-protected and non-SPM-protected binaries.
///
/// In non-SPM-protected variants the application needs to prove against the library that it has a
/// valid secret key which enables it to start the \neurayLibraryName.
class ILibrary_authenticator : public mi::base::Interface_declare<0x5a7d010a, 0x2a65, 0x43da, 0x92,
                                 0xf2, 0xcd, 0xd9, 0xc8, 0x4b, 0x10, 0xd2>
{
public:
  /// Convenience function implementing the full library authentication.
  ///
  /// The embedding application needs to pass in a pointer to the #mi::neuraylib::INeuray
  /// interface, the vendor key and the secret key. The function will then perform the
  /// authentication towards the library.
  ///
  /// The function is inline to avoid passing the secret key to the \neurayLibraryName which
  /// would enable eavesdroppers to get the key. Keys are supposed to be entered as delivered
  /// to the application writer (which are hex encoded byte arrays).
  ///
  /// \param library           A pointer to an instance of #mi::neuraylib::INeuray. The method
  ///                          attempts to authenticate this instance of the library.
  /// \param vendor_key        The vendor key assigned to the application writer.
  /// \param vendor_key_length The size of the vendor key.
  /// \param secret_key        The secret key provided to the application writer.
  /// \param secret_key_length The size of the secret key.
  /// \param count             The number of licenses to retrieve.
  /// \return                  Indicates errors in the authentication attempt. Returns 0 if there
  ///                          was no error, or negative values in case of errors. Note that a
  ///                          value of 0 does not imply that the key is valid. Key validity is
  ///                          checked during startup, i.e., in #mi::neuraylib::INeuray::start().
  inline static Sint32 authenticate(const INeuray* library, const char* vendor_key,
    Size vendor_key_length, const char* secret_key, Size secret_key_length, Sint32 count = 1);

  //  Returns a challenge from the library.
  //
  //  Asks the library for a challenge. The authentication process combines the challenge with the
  //  secret key and the vendor key to generate the correct response.
  //
  //  \note This method is internal and should not be called directly. Use the convenience method
  //        #authenticate() instead.
  //
  //  \param buffer          The library will store the challenge here.
  //  \param buffer_length   The actual size of the buffer. If the buffer is not big enough
  //                         generating the challenge generation will fail. Currently, the buffer
  //                         should be able to hold at least 32 bytes.
  //  \return                The necessary size of the needed buffer. The application needs to
  //                         check this value to determine whether its supplied buffer was big
  //                         enough, otherwise the challenge is not valid.
  virtual Size get_challenge(char* buffer, Size buffer_length) = 0;

  //  Submits the calculated response to a challenge.
  //
  //  In addition, the application has to provide a vendor key and a random salt. This is used to
  //  make attacks more difficult.
  //
  //  \note This method is internal and should not be called directly. Use the convenience method
  //        #authenticate() instead.
  //
  //  \param response          The calculated response.
  //  \param response_length   The size of the response.
  //  \param vendor_key        The vendor key assigned to the application writer.
  //  \param vendor_key_length The size of the vendor key.
  //  \param salt              A random salt. Should be a random array of bytes.
  //  \param salt_length       The size of the salt.
  //  \param count             The number of licenses to retrieve.
  virtual void submit_challenge_response(const char* response, Size response_length,
    const char* vendor_key, Size vendor_key_length, const char* salt, Size salt_length,
    Sint32 count) = 0;

  /// Indicates whether the license provided for authentication is a time-limited license.
  ///
  /// This method can only be called after \NeurayProductName has been started or after
  /// #mi::neuraylib::ILibrary_authenticator::is_flexnet_license_available().
  ///
  /// \note A time-limited license might be either a trial license or a long-running license.
  ///
  /// \return                  \c true if license is a time-limited license, \c false otherwise
  ///                          (including \NeurayProductName has not yet been started)
  virtual bool is_trial_license() const = 0;

  /// Returns the number of seconds left for time-limited licenses.
  ///
  /// This method returns sensible data only when called after \NeurayProductName has been
  /// started or after #mi::neuraylib::ILibrary_authenticator::is_flexnet_license_available().
  ///
  /// \note A time-limited license might be either a trial license or a long-running license.
  ///
  /// \return                 The number of seconds left before a time-limited license expires,
  ///                         or #mi::base::numeric_traits<mi::base::Uint64>::max() for permanent
  ///                         licenses (or \NeurayProductName has not yet been started).
  virtual Uint64 get_trial_seconds_left() const = 0;

  /// Returns the host ID of the machine the program is running on.
  ///
  /// The host ID is a unique identifier of the machine which can be used to lock a license to a
  /// machine.
  virtual const IString* get_host_id() const = 0;

  /// Returns the last error message related to authentication.
  ///
  /// For example, if authentication via FlexNet failed, i.e., #mi::neuraylib::INeuray::start()
  /// returns -6, then there might be a message providing a more detailed error description,
  /// originating from the FlexNet utilities.
  ///
  /// \return   The last error message, or \c NULL if there is no such error message available.
  virtual const IString* get_last_error_message() const = 0;

  /// Sets the expected FlexNet license file location.
  ///
  /// To check out a feature, a FlexNet enabled application must first locate the license file.
  /// This function sets the default location where to start looking for the license file. The @
  /// symbol cannot be used for file paths, as it is used for specifying FlexNet server addresses.
  /// Please note that this function should only be used after the response was submitted using
  /// #mi::neuraylib::ILibrary_authenticator::authenticate(), otherwise the return value is
  /// meaningless.
  ///
  /// \return    \c true if a valid FlexNet (non-trial) license was found in this path or
  ///            eventually configured environment variables.
  virtual bool set_flexnet_default_license_path(const char* path) = 0;

  /// Sets the content of the FlexNet trial license.
  ///
  /// Similar to #set_flexnet_default_license_path(), except that for trial licenses the license
  /// data is passed in memory.
  virtual void set_flexnet_trial_license_data(const Uint8* data, Size size) = 0;

  /// Indicates whether a valid FlexNet license for the submitted response is available.
  ///
  /// Please note that this function can only be used after the response was submitted with the
  /// call #mi::neuraylib::ILibrary_authenticator::authenticate().
  ///
  /// \return    \c true if a valid FlexNet license is available, \c false if no valid FlexNet
  ///            license is available, no response has been submitted yet, or the license has
  ///            already been checked out.
  virtual bool is_flexnet_license_available() const = 0;
};

/*@}*/ // end group mi_neuray_configuration

namespace detail
{

// Generates a nonce.
//
// \param[out] buffer       The buffer for the nonce. Needs to be able to hold 32 bytes.
static void generate_nonce(char* buffer);

// Generates the response for a challenge, salt, and secret key.
//
// \param salt              The salt (32 bytes).
// \param challenge         The challenge (32 bytes).
// \param secret_key        The secret key used to calculate the response.
// \param secret_key_length The size of the secret key.
// \param[out] response     The buffer for the response. Needs to be able to hold 32 bytes.
static void calculate_response(const char* salt, const char* challenge, const char* secret_key,
  Size secret_key_length, char* response);

// Computes a SHA256 hash value.
//
// \param input           The input for which to compute the SHA256 hash value.
// \param input_length    The size of the input.
// \param[out] buffer     The buffer for the SHA256 hash value. Needs to be able to hold 32 bytes.
static void sha256(const char* input, unsigned int input_length, char* buffer);

} // namespace detail

inline Sint32 ILibrary_authenticator::authenticate(const INeuray* library, const char* vendor_key,
  Size vendor_key_length, const char* secret_key, Size secret_key_length, Sint32 count)
{
  if (!library)
    return -1;

  base::Handle<ILibrary_authenticator> authenticator(
    library->get_api_component<ILibrary_authenticator>());
  if (!authenticator.is_valid_interface())
    return -2;

  char challenge[32];
  memset(&challenge[0], 0, 32);
  if (authenticator->get_challenge(challenge, sizeof(challenge)) > sizeof(challenge))
    return -3;

  char salt[32];
  detail::generate_nonce(salt);

  char response[32];
  detail::calculate_response(salt, challenge, secret_key, secret_key_length, response);

  authenticator->submit_challenge_response(
    response, sizeof(response), vendor_key, vendor_key_length, salt, sizeof(salt), count);
  return 0;
}

namespace detail
{

void calculate_response(const char* salt, const char* challenge, const char* secret_key,
  Size secret_key_length, char* response)
{
  if (secret_key_length > 1024u * 10u)
    return;
  // salt = 32 bytes, challenge = 32 bytes
  Size total_len = 32u + 32u + secret_key_length;
  char* buffer = new char[total_len];
  memcpy(buffer, salt, 32);
  memcpy(buffer + 32, challenge, 32);
  memcpy(buffer + 64, secret_key, secret_key_length);
  sha256(buffer, static_cast<Uint32>(total_len), response);
  delete[] buffer;
}

void generate_nonce(char* buffer)
{
  if (buffer == 0)
    return;
  Uint64 number = 0;
#ifdef MI_PLATFORM_WINDOWS
  srand(GetTickCount());
  LARGE_INTEGER tmp;
  QueryPerformanceCounter(&tmp);
  number += tmp.QuadPart + GetCurrentProcessId() + GetTickCount();
#else
  // Note: icc 13.1 report warning here as an implicit conversion,
  // but this is an explicit conversion.
  srand(static_cast<unsigned>(time(0)));
  struct timeval tv;
  gettimeofday(&tv, 0);
  number += static_cast<Uint64>(tv.tv_sec + tv.tv_usec);
#endif
  char buf[sizeof(Uint32) + 3 * sizeof(Uint64)] = { 0 };
  int r = rand();
  memcpy(buf, &r, sizeof(Uint32));
  memcpy(buf + sizeof(Uint32), &number, sizeof(Uint64));
  memcpy(buf + sizeof(Uint32) + sizeof(Uint64), &number, sizeof(Uint64));
  number += static_cast<Uint64>(rand());
  memcpy(buf + sizeof(Uint32) + 2 * sizeof(Uint64), &number, sizeof(Uint64));
  sha256(buf, static_cast<Uint32>(sizeof(buf)), buffer);
}

// Table of round constants.
// First 32 bits of the fractional parts of the cube roots of the first 64 primes 2..311
static const Uint32 sha256_constants[] = { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
  0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
  0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
  0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
  0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
  0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

// Reverses the byte order of the bytes of \p x
static Uint32 flip32(Uint32 x)
{
  return ((((x)&0xff000000) >> 24) | (((x)&0x00ff0000) >> 8) | (((x)&0x0000ff00) << 8) |
    (((x)&0x000000ff) << 24));
}

// Rotates the bits of \p x to the right by \p y bits
static Uint32 rightrotate(Uint32 x, Uint32 y)
{
  return ((x >> y) | (x << (32 - y)));
}

void sha256(const char* input, unsigned int input_length, char* buffer)
{
  if ((input_length <= 0) || (input == 0) || (buffer == 0))
    return;

  // First 32 bits of the fractional parts of the square roots of the first 8 primes 2..19
  Uint32 state[] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c,
    0x1f83d9ab, 0x5be0cd19 };

  // k is the number of '0' bits >= 0 such that the resulting message length is 448 (mod 512)
  unsigned int r = (input_length * 8 + 1) % 512;
  unsigned int k = r > 448 ? 960 - r : 448 - r;

  unsigned int pos = 0;
  for (unsigned int chunk = 0; k != 0; ++chunk)
  {

    Uint32 W[64] = { 0 };
    Uint8* ptr = reinterpret_cast<Uint8*>(W);
    unsigned int to_copy = input_length - pos;
    to_copy = to_copy > 64 ? 64 : to_copy;
    if (to_copy > 0)
    {
      memcpy(ptr, input + pos, to_copy);
      pos += to_copy;
    }

    // If we are at the end of input message
    if (pos == input_length)
    {
      // If we still have not padded and have space to add a 1, add it
      if ((k > 0) && (pos / 64 == chunk))
        ptr[pos % 64] |= static_cast<Uint8>(0x80);
      // If we can pad and still have space to add the length, add it
      if ((pos * 8 + 1 + k) - (chunk * 512) <= 448)
      {
        Uint64 value = input_length * 8;
        ptr = reinterpret_cast<Uint8*>(&W[14]);
        // Note: icc 13.1 report warning for the following
        // code as an implicit conversion, but this is an
        // explicit conversion.
        ptr[0] = static_cast<Uint8>((value >> 56) & 0xff);
        ptr[1] = static_cast<Uint8>((value >> 48) & 0xff);
        ptr[2] = static_cast<Uint8>((value >> 40) & 0xff);
        ptr[3] = static_cast<Uint8>((value >> 32) & 0xff);
        ptr[4] = static_cast<Uint8>((value >> 24) & 0xff);
        ptr[5] = static_cast<Uint8>((value >> 16) & 0xff);
        ptr[6] = static_cast<Uint8>((value >> 8) & 0xff);
        ptr[7] = static_cast<Uint8>(value & 0xff);
        k = 0;
      }
    }

    // Flip to big endian
    for (int i = 0; i < 16; ++i)
      W[i] = flip32(W[i]);

    // Extend the sixteen 32-bit words into 64 32-bit words
    for (Uint32 i = 16; i < 64; ++i)
    {
      Uint32 s0 = rightrotate(W[i - 15], 7) ^ rightrotate(W[i - 15], 18) ^ (W[i - 15] >> 3);
      Uint32 s1 = rightrotate(W[i - 2], 17) ^ rightrotate(W[i - 2], 19) ^ (W[i - 2] >> 10);
      W[i] = W[i - 16] + s0 + W[i - 7] + s1;
    }

    // Initialize hash value for this chunk
    Uint32 a = state[0];
    Uint32 b = state[1];
    Uint32 c = state[2];
    Uint32 d = state[3];
    Uint32 e = state[4];
    Uint32 f = state[5];
    Uint32 g = state[6];
    Uint32 h = state[7];

    for (Uint32 j = 0; j < 64; ++j)
    {
      Uint32 s0 = rightrotate(a, 2) ^ rightrotate(a, 13) ^ rightrotate(a, 22);
      Uint32 maj = (a & b) ^ (a & c) ^ (b & c);
      Uint32 t2 = s0 + maj;
      Uint32 s1 = rightrotate(e, 6) ^ rightrotate(e, 11) ^ rightrotate(e, 25);
      Uint32 ch = (e & f) ^ ((~e) & g);
      Uint32 t1 = h + s1 + ch + sha256_constants[j] + W[j];

      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }

    // Add this chunk's hash value to result so far
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
  }

  // Flip to little endian
  for (int i = 0; i < 8; ++i)
    state[i] = flip32(state[i]);

  memcpy(buffer, reinterpret_cast<char*>(state), 32);
}

} // namespace detail

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_ILIBRARY_AUTHENTICATION_H
