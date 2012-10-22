/**
 * @file node_test.c
 * @date 30 juil. 2012
 * @author nicolas.carrier@parrot.com
 * @brief Unit tests for rs_node module
 *
 * Copyright (C) 2012 Parrot S.A.
 */
#include <limits.h>
#include <inttypes.h>

#include <CUnit/Basic.h>

#include <rs_node.h>
#include "../src/rs_utils.h"

#include <fautes.h>
#include <fautes_utils.h>

struct int_node {
	int val;
	rs_node_t node;
};

typedef struct int_node int_node_t;

#define to_int_node(p) container_of(p, int_node_t, node)

static int int_node_test_equals(rs_node_t *node_a, void *int_node_b)
{
	int_node_t *int_node_a = to_int_node(node_a);

	if (NULL == node_a || NULL == int_node_b)
		return 0;

	return 0 == (int_node_a->val - ((int_node_t *)int_node_b)->val);
}

static RS_NODE_MATCH_MEMBER(int_node_t, val, node)

static void testRS_NODE_HEAD(void)
{
	int_node_t int_node_a = {.val = 17,};
	int_node_t int_node_b = {.val = 42,};
	rs_node_t *list = NULL;
	rs_node_t *head = NULL;
	int err;

	/* normal use cases */
	head = rs_node_head(list);
	CU_ASSERT_PTR_NULL(head);

	err = rs_node_push(&list, &(int_node_a.node));
	CU_ASSERT_EQUAL_FATAL(err, 0);
	head = rs_node_head(list);
	CU_ASSERT_EQUAL(head, &(int_node_a.node));

	err = rs_node_push(&list, &(int_node_b.node));
	CU_ASSERT_EQUAL_FATAL(err, 0);
	head = rs_node_head(list);
	CU_ASSERT_EQUAL(head, &(int_node_b.node));

	/* error cases : none */
}

static void testRS_NODE_INSERT(void)
{
	int_node_t int_node_a = {.val = 17,};
	int_node_t int_node_b = {.val = 42,};
	int_node_t int_node_c = {.val = 666,};
	rs_node_t *list = NULL;
	rs_node_t *tmp = &(int_node_a.node);

	/* normal use cases */
	/* inserting to a NULL (empty) list returns a list composed of node */
	list = rs_node_insert(list, &(int_node_a.node));
	CU_ASSERT_EQUAL(list, &(int_node_a.node));
	CU_ASSERT_EQUAL(to_int_node(list)->val, 17);
	CU_ASSERT_PTR_NULL(list->next);
	CU_ASSERT_PTR_NULL(list->prev);

	list = rs_node_insert(list, &(int_node_b.node));
	CU_ASSERT_EQUAL(list, &(int_node_b.node));
	CU_ASSERT_EQUAL(to_int_node(list)->val, 42);
	CU_ASSERT_PTR_NULL(list->next->next);
	CU_ASSERT_EQUAL(list->next, &(int_node_a.node));
	CU_ASSERT_PTR_NULL(list->next->next);
	CU_ASSERT_EQUAL(list->next->prev, &(int_node_b.node));
	CU_ASSERT_PTR_NULL(list->next->prev->prev);



	list = rs_node_insert(list, &(int_node_c.node));
	CU_ASSERT_EQUAL(list, &(int_node_c.node));
	CU_ASSERT_EQUAL(to_int_node(list)->val, 666);

	/* inserting a NULL element returns the initial list unchanged */
	tmp = rs_node_insert(list, NULL);
	CU_ASSERT_EQUAL(list, tmp);

	/* error cases : none */
}

static void testRS_NODE_PUSH(void)
{
	int_node_t int_node_a = {.val = 17,};
	int_node_t int_node_b = {.val = 42,};
	int_node_t int_node_c = {.val = 666,};
	rs_node_t *list = NULL;
	int err;

	/* normal use cases */
	err = rs_node_push(&list, &(int_node_a.node));
	CU_ASSERT_EQUAL(err, 0);
	CU_ASSERT_EQUAL(to_int_node(list)->val, 17);
	err = rs_node_push(&list, &(int_node_b.node));
	CU_ASSERT_EQUAL(err, 0);
	CU_ASSERT_EQUAL(to_int_node(list)->val, 42);
	err = rs_node_push(&list, &(int_node_c.node));
	CU_ASSERT_EQUAL(err, 0);
	CU_ASSERT_EQUAL(to_int_node(list)->val, 666);

	/* pushing nothing, well... does nothing */
	err = rs_node_push(&list, NULL);
	CU_ASSERT_EQUAL(err, 0);

	/* error cases */
	err = rs_node_push(NULL, &(int_node_a.node));
	CU_ASSERT_NOT_EQUAL(err, 0);
}

static void testRS_NODE_POP(void)
{
	int_node_t int_node_a = {.val = 17,};
	int_node_t int_node_b = {.val = 42,};
	int_node_t int_node_c = {.val = 666,};
	rs_node_t *list = NULL;
	rs_node_t *node = NULL;

	rs_node_push(&list, &(int_node_a.node));
	rs_node_push(&list, &(int_node_b.node));
	rs_node_push(&list, &(int_node_c.node));

	/* normal use cases */
	node = rs_node_pop(&list);
	CU_ASSERT_EQUAL(node, &(int_node_c.node));
	CU_ASSERT_EQUAL(list, &(int_node_b.node));
	node = rs_node_pop(&list);
	CU_ASSERT_EQUAL(node, &(int_node_b.node));
	CU_ASSERT_EQUAL(list, &(int_node_a.node));
	node = rs_node_pop(&list);
	CU_ASSERT_EQUAL(node, &(int_node_a.node));
	CU_ASSERT_PTR_NULL(list);
	node = rs_node_pop(&list);
	CU_ASSERT_PTR_NULL(node);

	/* error cases : none */
}

static void testRS_NODE_COUNT(void)
{
	int_node_t int_node_a = {.val = 17,};
	int_node_t int_node_b = {.val = 42,};
	int_node_t int_node_c = {.val = 666,};
	rs_node_t *list = NULL;
	unsigned count;

	/* normal use cases */
	count = rs_node_count(list);
	CU_ASSERT_EQUAL(0, count);
	rs_node_push(&list, &(int_node_a.node));
	count = rs_node_count(list);
	CU_ASSERT_EQUAL(1, count);
	rs_node_push(&list, &(int_node_b.node));
	count = rs_node_count(list);
	CU_ASSERT_EQUAL(2, count);
	rs_node_push(&list, &(int_node_c.node));
	count = rs_node_count(list);
	CU_ASSERT_EQUAL(3, count);

	/* count from middle of the list is possible */
	count = rs_node_count(&(int_node_a.node));
	CU_ASSERT_EQUAL(1, count);
	count = rs_node_count(&(int_node_b.node));
	CU_ASSERT_EQUAL(2, count);
	count = rs_node_count(&(int_node_c.node));
	CU_ASSERT_EQUAL(3, count);

	/* error cases : none */
}

static void testRS_NODE_NEXT(void)
{
	int_node_t int_node_a = {.val = 17,};
	int_node_t int_node_b = {.val = 42,};
	int_node_t int_node_c = {.val = 666,};
	rs_node_t *list = NULL;
	rs_node_t *node = NULL;

	rs_node_push(&list, &(int_node_a.node));
	rs_node_push(&list, &(int_node_b.node));
	rs_node_push(&list, &(int_node_c.node));

	/* normal use cases */
	CU_ASSERT_EQUAL(to_int_node(list)->val, 666);
	node = rs_node_next(list);
	CU_ASSERT_EQUAL(to_int_node(node)->val, 42);
	node = rs_node_next(node);
	CU_ASSERT_EQUAL(to_int_node(node)->val, 17);
	node = rs_node_next(node);
	CU_ASSERT_PTR_NULL(node);

	/* error cases : none */
}

static void testRS_NODE_PREV(void)
{
	int_node_t int_node_a = {.val = 17,};
	int_node_t int_node_b = {.val = 42,};
	int_node_t int_node_c = {.val = 666,};
	rs_node_t *list = NULL;
	rs_node_t *node = NULL;

	rs_node_push(&list, &(int_node_a.node));
	rs_node_push(&list, &(int_node_b.node));
	rs_node_push(&list, &(int_node_c.node));

	/* normal use cases */
	node = rs_node_prev(NULL);
	CU_ASSERT_PTR_NULL(node);
	node = rs_node_prev(&(int_node_a.node));
	CU_ASSERT(int_node_test_equals(node, &int_node_b));
	node = rs_node_prev(&(int_node_b.node));
	CU_ASSERT(int_node_test_equals(node, &int_node_c));
	node = rs_node_prev(&(int_node_c.node));
	CU_ASSERT_PTR_NULL(node);

	/* error cases : none */
}

static void testRS_NODE_FIND_MATCH(void)
{
	int_node_t int_node_a = {.val = 17,};
	int_node_t int_node_b = {.val = 42,};
	int_node_t int_node_c = {.val = 666,};
	int val;
	rs_node_t *haystack = NULL;
	rs_node_t *needle = NULL;

	rs_node_push(&haystack, &(int_node_a.node));
	rs_node_push(&haystack, &(int_node_b.node));
	rs_node_push(&haystack, &(int_node_c.node));

	/* normal use cases */
	val = 17;
	needle = rs_node_find_match(NULL, match_val, &val);
	CU_ASSERT_PTR_NULL(needle);

	val = 17;
	needle = rs_node_find_match(haystack, match_val, &val);
	CU_ASSERT(int_node_test_equals(needle, &int_node_a));
	val = 42;
	needle = rs_node_find_match(haystack, match_val, &val);
	CU_ASSERT(int_node_test_equals(needle, &int_node_b));
	val = 666;
	needle = rs_node_find_match(haystack, match_val, &val);
	CU_ASSERT(int_node_test_equals(needle, &int_node_c));

	needle = rs_node_find_match(haystack, match_val, &val);
	CU_ASSERT(int_node_test_equals(needle, &int_node_c));

	needle = rs_node_find_match(haystack, NULL, &val);
	CU_ASSERT_PTR_NULL(needle);

	needle = rs_node_find_match(haystack, match_val, NULL);
	CU_ASSERT_PTR_NULL(needle);

	/* error cases : none */
}

static void testRS_NODE_REMOVE(void)
{
	int_node_t int_node_a = {.val = 17,};
	int_node_t int_node_b = {.val = 42,};
	int_node_t int_node_c = {.val = 666,};
	int_node_t int_node_unknown = {.val = 666,};
	rs_node_t *list = NULL;
	rs_node_t *node = NULL;

	rs_node_push(&list, &(int_node_a.node));
	rs_node_push(&list, &(int_node_b.node));
	rs_node_push(&list, &(int_node_c.node));

	/* normal use cases */
	node = rs_node_remove(list, &(int_node_b.node));
	CU_ASSERT(int_node_test_equals(node, &int_node_b));

	node = rs_node_remove(list, &(int_node_unknown.node));
	CU_ASSERT_PTR_NULL(node);

	node = rs_node_remove(list, &(int_node_c.node));
	CU_ASSERT(int_node_test_equals(node, &int_node_c));

	node = rs_node_remove(NULL, &(int_node_a.node));
	CU_ASSERT_PTR_NULL(node);

	node = rs_node_remove(list, NULL);
	CU_ASSERT_PTR_NULL(node);

	/* error cases : none */
}

static void testRS_NODE_REMOVE_MATCH(void)
{
	int val;
	int_node_t int_node_a = {.val = 17,};
	int_node_t int_node_b = {.val = 42,};
	int_node_t int_node_c = {.val = 666,};
	rs_node_t *list = NULL;
	rs_node_t *node = NULL;

	rs_node_push(&list, &(int_node_a.node));
	rs_node_push(&list, &(int_node_b.node));
	rs_node_push(&list, &(int_node_c.node));

	/* normal use cases */
	val = 42;
	node = rs_node_remove_match(list, match_val, &val);
	CU_ASSERT(int_node_test_equals(node, &int_node_b));

	val = 911;
	node = rs_node_remove_match(list, match_val, &val);
	CU_ASSERT_PTR_NULL(node);

	val = 666;
	node = rs_node_remove_match(list, match_val, &val);
	CU_ASSERT(int_node_test_equals(node, &int_node_c));

	val = 17;
	node = rs_node_remove_match(NULL, match_val, &val);
	CU_ASSERT_PTR_NULL(node);

	node = rs_node_remove_match(list, match_val, NULL);
	CU_ASSERT_PTR_NULL(node);

	node = rs_node_remove_match(list, NULL, &val);
	CU_ASSERT_PTR_NULL(node);

	/* error cases : none */
}

static void testRS_NODE_FOREACH(void)
{
	int val = 2;
	int ret;
	int_node_t int_node_a = {.val = 17,};
	int_node_t int_node_b = {.val = 42,};
	int_node_t int_node_c = {.val = 666,};
	rs_node_t *list = NULL;
	int cb(rs_node_t __attribute__((unused))*node,
			void __attribute__((unused))*data)
	{
		int *v = data;

		int_node_t *in = to_int_node(node);
		in->val *= *v;

		return 0;
	};

	rs_node_push(&list, &(int_node_a.node));
	rs_node_push(&list, &(int_node_b.node));
	rs_node_push(&list, &(int_node_c.node));

	/* normal use cases */
	ret = rs_node_foreach(list, cb, &val);
	CU_ASSERT_EQUAL(ret, 0);
	CU_ASSERT_EQUAL(int_node_a.val, 34);
	CU_ASSERT_EQUAL(int_node_b.val, 84);
	CU_ASSERT_EQUAL(int_node_c.val, 1332);

	/* error use cases */
}

static const test_t tests[] = {
		{
				.fn = testRS_NODE_HEAD,
				.name = "rs_node_head"
		},
		{
				.fn = testRS_NODE_INSERT,
				.name = "rs_node_insert"
		},
		{
				.fn = testRS_NODE_PUSH,
				.name = "rs_node_push"
		},
		{
				.fn = testRS_NODE_POP,
				.name = "rs_node_pop"
		},
		{
				.fn = testRS_NODE_COUNT,
				.name = "rs_node_count"
		},
		{
				.fn = testRS_NODE_NEXT,
				.name = "rs_node_next"
		},
		{
				.fn = testRS_NODE_PREV,
				.name = "rs_node_prev"
		},
		{
				.fn = testRS_NODE_FIND_MATCH,
				.name = "rs_node_find_match"
		},
		{
				.fn = testRS_NODE_REMOVE,
				.name = "rs_node_remove"
		},
		{
				.fn = testRS_NODE_REMOVE_MATCH,
				.name = "rs_node_remove_match"
		},
		{
				.fn = testRS_NODE_FOREACH,
				.name = "rs_node_foreach"
		},

		/* NULL guard */
		{.fn = NULL, .name = NULL},
};

static int init_node_suite(void)
{
	return 0;
}

static int clean_node_suite(void)
{

	return 0;
}

suite_t node_suite = {
		.name = "rs_node",
		.init = init_node_suite,
		.clean = clean_node_suite,
		.tests = tests,
};