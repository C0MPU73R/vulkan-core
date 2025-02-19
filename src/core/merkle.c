// Copyright (c) 2019-2022, The Vulkan Developers.
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

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include <sodium.h>

#include "common/util.h"

#include "merkle.h"

#include "crypto/cryptoutil.h"
#include "crypto/sha256d.h"

/*
 * Constructing a Merkle Tree requires passing a large allocated uint8_t that contains
 * a series of 32 byte hashes. (non-separated). A second parameter determines the number
 * of hashes the hash buffer contains.
 */
merkle_tree_t *construct_merkle_tree_from_leaves(uint8_t *hashes, uint32_t num_of_hashes)
{
  if (num_of_hashes < 1)
  {
    return NULL;
  }

  merkle_tree_t *tree = malloc(sizeof(merkle_tree_t));
  assert(tree != NULL);
  uint32_t num_of_nodes = 0;

  merkle_node_t **nodes = malloc(sizeof(merkle_node_t) * num_of_hashes);
  assert(nodes != NULL);
  construct_merkle_leaves_from_hashes(nodes, &num_of_nodes, hashes, num_of_hashes);

  while (num_of_nodes > 1)
  {
    collapse_merkle_nodes(nodes, &num_of_nodes);
  }

  tree->root = nodes[0];
  free(nodes);
  return tree;
}

/*
 * Loops through all of the hashes, and creates leave nodes for each hash.
 */
int construct_merkle_leaves_from_hashes(merkle_node_t **nodes, uint32_t *num_of_nodes, uint8_t *hashes, uint32_t num_of_hashes)
{
  assert(nodes != NULL);
  for (uint32_t i = 0; i < num_of_hashes; i++)
  {
    merkle_node_t *node = malloc(sizeof(merkle_node_t));
    assert(node != NULL);
    node->left = NULL;
    node->right = NULL;
    memcpy(node->hash, &hashes[i * HASH_SIZE], HASH_SIZE);
    nodes[i] = node;
  }

  *num_of_nodes = num_of_hashes;
  return 0;
}

/*
 * Collapses the list of nodes into a smaller list of parent nodes that are hashes of 2 child nodes.
 */
int collapse_merkle_nodes(merkle_node_t **nodes, uint32_t *num_of_nodes)
{
  assert(nodes != NULL);
  uint32_t current_node_idx = 0;
  merkle_node_t **temp_nodes = malloc(sizeof(merkle_node_t) * (*num_of_nodes));
  assert(temp_nodes != NULL);

  for (uint32_t i = 0; i < (*num_of_nodes - 1); i += 2)
  {
    temp_nodes[current_node_idx] = construct_merkle_node(nodes[i], nodes[i + 1]);
    current_node_idx++;
  }

  if (*num_of_nodes % 2 != 0)
  {
    temp_nodes[current_node_idx] = construct_merkle_node(nodes[*num_of_nodes - 1], nodes[*num_of_nodes - 1]);
    current_node_idx++;
  }

  for (uint32_t i = 0; i < current_node_idx; i++)
  {
    nodes[i] = temp_nodes[i];
  }

  *num_of_nodes = current_node_idx;
  free(temp_nodes);
  return 0;
}

/*
 * Creates a MerkleNode that contains the hash of two MerkleNodes.
 */
merkle_node_t *construct_merkle_node(merkle_node_t *left, merkle_node_t *right)
{
  assert(left != NULL);
  assert(right != NULL);

  uint8_t *combined_hash = malloc(HASH_SIZE * 2);
  assert(combined_hash != NULL);
  uint8_t *node_hash = malloc(HASH_SIZE);

  memcpy(combined_hash, left->hash, HASH_SIZE);
  memcpy(combined_hash + HASH_SIZE, right->hash, HASH_SIZE);

  crypto_hash_sha256d(node_hash, combined_hash, HASH_SIZE * 2);

  merkle_node_t *node = malloc(sizeof(merkle_node_t));
  assert(node != NULL);
  memcpy(node->hash, node_hash, HASH_SIZE);
  node->left = left;
  node->right = right;

  free(combined_hash);
  free(node_hash);
  return node;
}

int compare_merkle_node(merkle_node_t *merkle_node, merkle_node_t *other_merkle_node)
{
  assert(merkle_node != NULL);
  assert(other_merkle_node != NULL);
  return compare_hash(merkle_node->hash, other_merkle_node->hash);
}

/*
 * Frees a merkle tree in DFS postorder traversal.
 */
void free_merkle_tree(merkle_tree_t *tree)
{
  assert(tree != NULL);
  free_merkle_node(tree->root);
  free(tree);
}

void free_merkle_node(merkle_node_t *node)
{
  assert(node != NULL);
  if ((node->left != NULL) && (node->left == node->right))
  {
    free_merkle_node(node->left);
  }
  else
  {
    if (node->left != NULL)
    {
      free_merkle_node(node->left);
      node->left = NULL;
    }

    if (node->right != NULL)
    {
      free_merkle_node(node->right);
      node->right = NULL;
    }
  }

  free(node);
}
