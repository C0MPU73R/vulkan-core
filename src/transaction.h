// Copyright (c) 2019, The Vulkan Developers.
//
// This file is part of Vulkan.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// You should have received a copy of the MIT License
// along with Vulkan. If not, see <https://opensource.org/licenses/MIT>.

#pragma once

#include <stdint.h>
#include <sodium.h>

#include "vulkan.pb-c.h"

/*
 * Transactions can contain multiple InputTXs and multiple OutputTXs.
 *
 * InputTXs are treated as sources of value, pooled together and then split apart into one or more OutputTXs
 * InputTXs reference a previous transaction hash where the value is coming from an earlier OutputTX (solidified in the blockchain)
 * InputTXs contain a signature of the InputTX header, as well as the public key that signed it.
 *   - The public key is converted into an Address to verify that the prev OutputTX was sent to the pubkey's address.
 *   - This "unlocks" the previous OutputTX to be used as this current InputTX.
 *   - The public key is used to verify the signature. (meaning the header has not been tampered.)
 *   - From this, we know that:
 *     - The person that is making this transaction owns the value designated by the transaction the InputTX(s) refer to.
 *     - The person confirms that this transaction should be taking place.
 *
 * Once an OutputTX is used as an InputTX, it is considered spent. (All value is used from a OutputTX when being used as input)
 * - If you don't want to spend everything from an InputTX, you can create a new OutputTX to send back to yourself as leftover-change.
 */

#include "wallet.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define TXIN_HEADER_SIZE (32 + 4)
#define TXOUT_HEADER_SIZE (32 + 4)

extern uint8_t zero_tx_hash[32];

typedef struct InputTransaction
{
  // --- Header
  uint8_t transaction[32]; // Previous tx hash/id
  uint32_t txout_index; // Referenced txout index in previous tx
  // ---

  uint8_t signature[crypto_sign_BYTES];
  uint8_t public_key[crypto_sign_PUBLICKEYBYTES];
} input_transaction_t;

typedef struct OutputTransaction
{
  uint64_t amount;
  uint8_t address[ADDRESS_SIZE];
} output_transaction_t;

typedef struct Transaction
{
  uint8_t id[32];
  uint8_t txin_count;
  uint8_t txout_count;
  input_transaction_t **txins;
  output_transaction_t **txouts;
} transaction_t;

int sign_txin(input_transaction_t *txin, transaction_t *tx, uint8_t *public_key, uint8_t *secret_key);
int get_txin_header(uint8_t *header, input_transaction_t *txin);
int get_txout_header(uint8_t *header, output_transaction_t *txout);
uint32_t get_tx_sign_header_size(transaction_t *tx);
uint32_t get_tx_header_size(transaction_t *tx);
int get_tx_sign_header(uint8_t *header, transaction_t *tx);

int valid_transaction(transaction_t *tx);
int is_generation_tx(transaction_t *tx);
int do_txins_reference_unspent_txouts(transaction_t *tx);

int compute_tx_id(uint8_t *header, transaction_t *tx);
int compute_self_tx_id(transaction_t *tx);

PTransaction *transaction_to_proto(transaction_t *tx);
PUnspentTransaction *unspent_transaction_to_proto(transaction_t *tx);
output_transaction_t *txout_from_proto(POutputTransaction *proto_txout);
transaction_t *transaction_from_proto(PTransaction *proto_tx);
int unspent_transaction_to_serialized(uint8_t **buffer, uint32_t *buffer_len, transaction_t *tx);
int proto_unspent_transaction_to_serialized(uint8_t **buffer, uint32_t *buffer_len, PUnspentTransaction *tx);
int transaction_to_serialized(uint8_t **buffer, uint32_t *buffer_len, transaction_t *tx);
transaction_t *transaction_from_serialized(uint8_t *buffer, uint32_t buffer_len);
PUnspentTransaction *unspent_transaction_from_serialized(uint8_t *buffer, uint32_t buffer_len);

int free_proto_transaction(PTransaction *proto_transaction);
int free_proto_unspent_transaction(PUnspentTransaction *proto_unspent_tx);
int free_transaction(transaction_t *tx);

#ifdef __cplusplus
}
#endif
