/*                      S H O W T R E E . C
 * BRL-CAD
 *
 * Copyright (c) 1990-2025 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */

/*		Display a boolean tree		*/

#include "./iges_struct.h"
#include "./iges_extern.h"

#define STKBLK 100	/* Allocation block size */

/* Some junk for this routines private node stack */
struct node **sstk_p;
int sjtop, sstklen;

/* some junk for this routines private character string stack */
char **stk;
int jtop, stklen;

/* The following are stack routines for character strings */

static void
Initastack(void)
{
    int i;

    jtop = (-1);
    stklen = STKBLK;
    stk = (char **)bu_malloc(stklen*sizeof(char *), "Initastack: stk");
    if (stk == NULL) {
	bu_log("Cannot allocate stack space\n");
	perror("Initastack");
	bu_exit(1, NULL);
    }
    for (i = 0; i < stklen; i++)
	stk[i] = NULL;
}


/* This function pushes a pointer onto the stack. */

static void
Apush(char *ptr)
{
    int i;

    jtop++;
    if (jtop == stklen) {
	stklen += STKBLK;
	stk = (char **)bu_realloc((char *)stk, stklen*sizeof(char *), "Apush: stk");
	if (stk == NULL) {
	    bu_log("Cannot reallocate stack space\n");
	    perror("Apush");
	    bu_exit(1, NULL);
	}
	for (i = jtop; i < stklen; i++)
	    stk[i] = NULL;
    }
    stk[jtop] = ptr;
}


/* This function pops the top of the stack. */


static char *
Apop(void)
{
    char *ptr;

    if (jtop == (-1))
	ptr = NULL;
    else {
	ptr = stk[jtop];
	jtop--;
    }

    return ptr;
}


/* Free the memory associated with the stack */
static void
Afreestack(void)
{

    jtop = (-1);
    stklen = 0;
    bu_free((char *)stk, "Afreestack: stk");
    return;
}


/* The following routines are stack routines for 'struct node' */
static void
Initsstack(void) /* initialize the stack */
{

    sjtop = (-1);
    sstklen = STKBLK;
    sstk_p = (struct node **)bu_malloc(sstklen*sizeof(struct node *), "Initsstack: sstk_p");
    if (sstk_p == NULL) {
	bu_log("Cannot allocate stack space\n");
	perror("Initsstack");
	bu_exit(1, NULL);
    }
}


/* This function pushes a pointer onto the stack. */

static void
Spush(struct node *ptr)
{

    sjtop++;
    if (sjtop == sstklen) {
	sstklen += STKBLK;
	sstk_p = (struct node **)bu_realloc((char *)sstk_p, sstklen*sizeof(struct node *), "Spush: sstk_p");
	if (sstk_p == NULL) {
	    bu_log("Cannot reallocate stack space\n");
	    perror("Spush");
	    bu_exit(1, NULL);
	}
    }
    sstk_p[sjtop] = ptr;
}


/* This function pops the top of the stack. */


static struct node *
Spop(void)
{
    struct node *ptr;

    if (sjtop == (-1))
	ptr = NULL;
    else {
	ptr = sstk_p[sjtop];
	sjtop--;
    }

    return ptr;
}


/* free memory associated with the stack, but not the pointed to nodes */
static void
Sfreestack(void)
{
    sjtop = (-1);
    bu_free((char *)sstk_p, "Sfreestack: sstk_p");
    return;
}

void
Showtree(struct node *root)
{
    struct node *ptr;
    char *opa, *opb, *tmp, oper[4];

    bu_strlcpy(oper, "   ", sizeof(oper));

    /* initialize both stacks */
    Initastack();
    Initsstack();

    ptr = root;
    while (1) {
	while (ptr != NULL) {
	    Spush(ptr);
	    ptr = ptr->left;
	}
	ptr = Spop();

	if (ptr == NULL) {
	    bu_log("Error in Showtree: Popped a null pointer\n");
	    Afreestack();
	    Sfreestack();
	    return;
	}

	if (ptr->op < 0) /* this is an operand, push its name */
	    Apush(dir[-(1+ptr->op)/2]->name);
	else {
	    /* this is an operator */
	    size_t size;
	    /* Pop the names of the operands */
	    opb = Apop();
	    opa = Apop();

	    size = strlen(opa) + strlen(opb) + 6;

	    /* construct the character string (opa ptr->op opb) */
	    tmp = (char *)bu_malloc(size, "Showtree: tmp");
	    if (ptr->parent)
		bu_strlcpy(tmp, "(", size);
	    else
		*tmp = '\0';
	    bu_strlcat(tmp, opa, size);
	    oper[1] = operators[ptr->op];

	    bu_strlcat(tmp, oper, size);
	    bu_strlcat(tmp, opb, size);
	    if (ptr->parent)
		bu_strlcat(tmp, ")", size);

	    /* push the character string representing the result */
	    Apush(tmp);
	}

	if (ptr == root) {
	    /* done! */
	    bu_log("%s\n", Apop()); /* print the result */

	    /* free some memory */
	    Afreestack();
	    Sfreestack();
	    return;
	}

	if (ptr->parent) {
	  if (ptr != ptr->parent->right)
	    ptr = ptr->parent->right;
	  else
	    ptr = NULL;
	} else {
	  ptr = NULL;
	}
    }
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
