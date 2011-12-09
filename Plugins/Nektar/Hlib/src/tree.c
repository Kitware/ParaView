/*
 * Binary Tree Code -- Maintains a Red/Black binary tree
 *
 * RCS Information
 * -----------------------------
 * $Author: ssherw $
 * $Date: 2006/05/08 09:59:30 $
 * $Source: /homedir/cvs/Nektar/Hlib/src/tree.c,v $
 * $Revision: 1.2 $
 * ----------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tree.h"
/* #include "hotel.h"*/

/* This is a "sentinel" node.  It serves as the boundary condition in a   *
 * tree.  Everything that normally points to NULL instead points to this  *
 * node.                                                                  */

static Node nil[] = { { "nil", Black, NULL, NULL, NULL} };

/* Private Functions */

static Node*
  i_tree_search      (Node *x, char *key);

static void
  i_tree_walk        (Node *x, PFV show),
  i_tree_insert      (Tree *T, Node *x),
  rotate_left        (Tree *T, Node *x),
  rotate_right       (Tree *T, Node *x);

/* ----------------------------------------------------------------------- *
 * Access functions                                                        *
 *                                                                         *
 * These functions query the tree, printing the values of nodes or search- *
 * ing for nodes with specific keys (=, min, max).                         *
 * ----------------------------------------------------------------------- */

/* Walk the tree, printing the value of each node */

void tree_walk (Tree *T)
{
  i_tree_walk (T->root, T->show);
  return;
}

static void i_tree_walk (Node *x, PFV show)
{
  if (x != nil) {
    i_tree_walk (x->left,  show);
    (*show)     (x->other);
    i_tree_walk (x->right, show);
  }
  return;
}

/* Search the tree for a node with the given key */

Node* tree_search (Node *x, char *key)
{ return i_tree_search (x, key); }

static Node* i_tree_search (Node *x, char *key)
{
  int compare;

  while (x != nil && (compare = strcmp(key,x->name)))
    x = (compare < 0) ? x->left : x->right;

  return (x == nil) ? (Node*) NULL : x;
}

/* Find the node with the minimum key */

Node* tree_minimum (Node *x)
{
  if (x != nil)
    while (x->left != nil)
      x = x->left;

  return x;
}

/* Find the node with the maximum key */

Node* tree_maximum (Node *x)
{
  if (x != nil)
    while (x->right != nil)
      x = x->right;

  return x;
}

/* Find the successor of a given node */

Node* tree_successor (Node *x)
{
  Node *y = (Node *) NULL;

  if (x != nil) {
    if (x->right != nil)
      return tree_minimum (x->right);

    y = x->parent;

    while (y != nil && (x = y->right) != nil) {
      x = y;
      y = y->parent;
    }
  }
  return y;
}

/* ----------------------------------------------------------------------- *
 * tree_insert() -- Insert a node                                          *
 *                                                                         *
 * This function inserts a node into the tree.  The new node is inserted   *
 * at the edge of the tree and colored Red, then the tree is corrected to  *
 * maintain the properties of a Red-Black tree.                            *
 * ----------------------------------------------------------------------- */

void tree_insert (Tree *T, Node *x)
{
  Node *y;
  Node *px, *gpx;

  i_tree_insert (T, x);  x->color = Red;

  while (x != T->root && x->parent->color == Red) {
    if ((px = x->parent) == (gpx = x->parent->parent)->left) {

    /* ............ Left Branch ............. */

      if ((y = gpx->right) == nil)
  y->parent = gpx;             /* Initialize the Nil node */

      if (y->color == Red) {
  y  ->color = Black;
  px ->color = Black;
  gpx->color = Red;
  x          = gpx;
      } else {
  if (x == px->right)
    rotate_left (T, x = px);
  x->parent->color         = Black;
  x->parent->parent->color = Red;
  rotate_right (T, x->parent->parent);
      }
    } else {

    /* ............ Right Branch ............. */

      if ((y = gpx->left) == nil)
  y->parent = gpx;             /* Initialize the Nil node */

      if (y->color == Red) {
  y  ->color = Black;
  px ->color = Black;
  gpx->color = Red;
  x          = gpx;
      } else {
  if (x == px->left)
    rotate_right (T, x = px);
  x->parent->color         = Black;
  x->parent->parent->color = Red;
  rotate_left (T, x->parent->parent);
      }
    }
  }

  T->root->color = nil->color = Black;   /* Reset the root and sentinel */

  return;
}

static void i_tree_insert (Tree *T, Node *z)
{
  Node *x = T->root,
       *y = nil;

  while (x != nil) {
    y = x;
    x = (strcmp(z->name, x->name) < 0) ? x->left : x->right;
  }

  z->parent  = y;

  if (y == nil)
    T->root  = z;
  else if (strcmp(z->name, y->name) < 0)
    y->left  = z;
  else
    y->right = z;

  return;
}

/* ----------------------------------------------------------------------- *
 * Rotations -- Do a rotation around a given node                          *
 *                                                                         *
 *                              Right(T,y)                                 *
 *                       y    ------------->    x                          *
 *                      / \                    / \                         *
 *                     x   c                  a   y                        *
 *                    / \     <-------------     / \                       *
 *                   a   b      Left(T,x)       b   c                      *
 *                                                                         *
 * ----------------------------------------------------------------------- */

static void rotate_left (Tree *T, Node *x)
{
  Node *y;

  y        = x->right;
  x->right = y->left;     /* Turn y's left subtree into x's right subtree */

  if (y->left != nil)
    y->left->parent = x;
  y->parent = x->parent;  /* Link x's parent to y */

  if (x->parent == nil)
    T->root = y;
  else if (x == x->parent->left)
    x->parent->left  = y;
  else
    x->parent->right = y;

  y->left   = x;          /* Put x on y's left */
  x->parent = y;

  return;
}

static void rotate_right (Tree *T, Node *x)
{
  Node *y;

  y        = x->left;
  x->left  = y->right;    /* Turn y's left subtree into x's right subtree */

  if (y->right != nil)
    y->right->parent = x;
  y->parent = x->parent;  /* Link x's parent to y */

  if (x->parent == nil)
    T->root = y;
  else if (x == x->parent->right)
    x->parent->right = y;
  else
    x->parent->left  = y;

  y->right  = x;          /* Put x on y's right */
  x->parent = y;

  return;
}

/* Allocate storage for a new node and initialize it's pointers */

Node *create_node (char *key)
{
  Node *new_node;

  new_node         = (Node *) calloc (1, sizeof(Node));
  new_node->color  = Black;
  new_node->left   = nil;
  new_node->right  = nil;
  new_node->parent = nil;
  new_node->name   = strdup (key);

  return new_node;
}

Tree *create_tree (PFV show, PFV kill)
{
  Tree *new_tree;

  new_tree       = (Tree*) malloc (sizeof(Tree));
  new_tree->root = nil;
  new_tree->show = show;
  new_tree->kill = kill;

  return new_tree;
}
