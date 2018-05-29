#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../include/sfmm.h"



/**
 *  HERE ARE OUR TEST CASES NOT ALL SHOULD BE GIVEN STUDENTS
 *  REMINDER MAX ALLOCATIONS MAY NOT EXCEED 4 * 4096 or 16384 or 128KB
 */

 Test(sf_memsuite, Malloc_an_Integer, .init = sf_mem_init, .fini = sf_mem_fini) {
 	int *x = sf_malloc(sizeof(int));
 	*x = 4;
 	cr_assert(*x == 4, "Failed to properly sf_malloc space for an integer!");
 	sf_free(x);
 }

 Test(sf_memsuite, Free_block_check_header_footer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
 	void *pointer = sf_malloc(sizeof(short));
 	sf_free(pointer);
 	pointer = pointer - 8;
 	sf_header *sfHeader = (sf_header *) pointer;
 	cr_assert(sfHeader->alloc == 0, "Alloc bit in header is not 0!\n");
 	sf_footer *sfFooter = (sf_footer *) (pointer - 8 + (sfHeader->block_size << 4));
 	cr_assert(sfFooter->alloc == 0, "Alloc bit in the footer is not 0!\n");
 	sf_free(pointer);
 }

 Test(sf_memsuite, PaddingSize_Check_char, .init = sf_mem_init, .fini = sf_mem_fini) {
 	void *pointer = sf_malloc(sizeof(char));
 	pointer = pointer - 8;
 	sf_header *sfHeader = (sf_header *) pointer;
 	cr_assert(sfHeader->padding_size == 15, "Header padding size is incorrect for malloc of a single char!\n");
 	sf_free(pointer);
 }

 Test(sf_memsuite, Check_next_prev_pointers_of_free_block_at_head_of_list, .init = sf_mem_init, .fini = sf_mem_fini) {
 	int *x = sf_malloc(4);
 	memset(x, 0, 4);
 	cr_assert(freelist_head->next == NULL);
 	cr_assert(freelist_head->prev == NULL);
 	sf_free(x);
 }

 Test(sf_memsuite, Coalesce_no_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
 	void *x = sf_malloc(4);
 	void *y = sf_malloc(4);
 	memset(y, 0xFF, 4);
 	sf_free(x);
 	cr_assert(freelist_head == x - 8);
 	sf_free_header *headofx = (sf_free_header*) (x - 8);
 	sf_footer *footofx = (sf_footer*) (x - 8 + (headofx->header.block_size << 4)) - 8;

 	sf_blockprint((sf_free_header*)((void*)x - 8));
    // All of the below should be true if there was no coalescing
 	cr_assert(headofx->header.alloc == 0);
 	cr_assert(headofx->header.block_size << 4 == 32);
 	cr_assert(headofx->header.padding_size == 0);

 	cr_assert(footofx->alloc == 0);
 	cr_assert(footofx->block_size << 4 == 32);
 	sf_free(y);
 }

/*
//############################################
// STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
// DO NOT DELETE THESE COMMENTS
//############################################
*/

// MY TEST CASES
 // Test(sf_memsuite, testing_head_of_freelist_head_with_reallocing_and_freeing_adjacent_block, .init = sf_mem_init, .fini = sf_mem_fini) {
 //    // printf("%s\n", "====================TEST 1=================");
 // 	write(1,"Fail",5);
 // 	void *a = sf_malloc(40);
 // 	void *b = sf_malloc(40);
 // 	sf_realloc(a,100);
 // 	sf_free(b);
 // 	cr_assert(freelist_head == a-8);
 // 	sf_free(a);
 // }
 Test(sf_memsuite, testing_mem_info_by_passing_null_value, .init = sf_mem_init, .fini = sf_mem_fini) {
    // printf("%s\n", "====================TEST 2=================");
 	write(1,"Fail",5);
 	struct mem_info * test = NULL;
 	cr_assert(sf_info(test) == -1);
 	write(1,"Test 2",7);
 }
 Test(sf_memsuite, testing_mem_info_by_passing_a_valid_info_struct_and_mallocing_with_sf_malloc, .init = sf_mem_init, .fini = sf_mem_fini) {
    // printf("%s\n", "====================TEST 3=================");
 	write(1,"Fail",5);
 	struct mem_info * test = sf_malloc(sizeof(struct mem_info));
    cr_assert(sf_info(test) == 0);//RETURN VALUE IS 0 IF ALLOCATION WAS SUCCESSFUL
    write(1,"Test 3",7);
}
Test(sf_memsuite, normal_allocating_but_random_freeing_and_coalesing, .init = sf_mem_init, .fini = sf_mem_fini) {
    // printf("%s\n", "====================TEST 4=================");
	write(1,"Fail",5);
	sf_malloc(40);
	void *a = sf_malloc(40);
	void *b = sf_malloc(40);
	void *c = sf_malloc(40);
	void *d = sf_malloc(40);
	sf_malloc(40);
	printf("%p\n", a);
	printf("%p\n", b);
	printf("%p\n", c);
	printf("%p\n", d);

	// sf_free(d);
	// cr_assert(freelist_head == d-8);

	// sf_free(a);
	// cr_assert(freelist_head == a-8);

	// sf_free(c);
	// cr_assert(freelist_head == c-8);

	// sf_free(b);
	// cr_assert(freelist_head == a-8);
	// write(1,"Test 4",7);
}

Test(sf_memsuite, freeing_invalid_address_by_freeing_twice, .init = sf_mem_init, .fini = sf_mem_fini) {
    // printf("%s\n", "====================TEST 5=================");
	write(1,"Fail",5);
	sf_malloc(40);
	void *a = sf_malloc(40);
	sf_malloc(40);
	printf("%p\n", a);

    // sf_free(a);//FREEDED ONCE
    // cr_assert(freelist_head == a-8); 

    // sf_free(a);//FREEING AGAIN
    // cr_assert(errno == 0);//ernno doesn't change
}
// Test(sf_memsuite, checking_if_value_remains_same_after_realloc, .init = sf_mem_init, .fini = sf_mem_fini) {
//     // printf("%s\n", "====================TEST 6=================");
// 	write(1,"Fail",5);
// 	char * d = sf_malloc(4);
// 	sf_malloc(4);

// 	*d = 'a';
// 	char * e = sf_realloc(d,32);

// 	cr_assert(*e == 'a');
// }
Test(sf_memsuite, freeing_in_the_middle_of_the_block, .init = sf_mem_init, .fini = sf_mem_fini) {
    // printf("%s\n", "====================TEST 7=================");
	write(1,"Fail",5);
	sf_malloc(40);
	void *a = sf_malloc(40);
	sf_malloc(40);
	printf("%p\n", a);
    // sf_free(a+10); //FREEING from 10 bytes in the middle of the block
    cr_assert(errno == 0);//errno doesn't change
}
// Test(sf_memsuite, reverse_freeing, .init = sf_mem_init, .fini = sf_mem_fini) {
//     // printf("%s\n", "====================TEST 8=================");
// 	write(1,"Fail",5);
// 	void * a = sf_malloc(10);
// 	void * b = sf_malloc(10);
// 	void * c = sf_malloc(10);
// 	void * d = sf_malloc(10);

// 	cr_assert(freelist_head == (d-8)+32);
// 	sf_free(d);
// 	cr_assert(freelist_head == (d-8));
// 	sf_free(c);
// 	cr_assert(freelist_head == (c-8));
// 	sf_free(b);
// 	cr_assert(freelist_head == (b-8));
// 	sf_free(a);
// 	cr_assert(freelist_head == (a-8));
// }
// Test(sf_memsuite, checking_for_error_code_while_mallocing_over_4_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
//     // printf("%s\n", "====================TEST 9=================");
//     for(int i =0; i < 5; i++){
//         sf_malloc(4080);
//         if(i == 4){
//             cr_assert(errno != 0);//MALLOCING OVER 4 PAGES SHOULD CAUSE ERROR
//         }
//         else{
//             cr_assert(errno == 0);//MALLOCING UNDER 4 PAGES SHOULD BE FINE
//         }
//         cr_assert(freelist_head == NULL);//
//     }
// }
Test(sf_memsuite, checking_free_list_with_full_allocation_of_4_pages_and_freeing, .init = sf_mem_init, .fini = sf_mem_fini) {
    // printf("%s\n", "====================TEST 10=================");
    void * a = sf_malloc((4096*4)-16);
    cr_assert(freelist_head == NULL);
    cr_assert(errno == 0);

    // sf_free(a);
    cr_assert(freelist_head == a-8);
    cr_assert(freelist_head->header.block_size << 4 == (4*4096));//Coalesced block size
    sf_blockprint(freelist_head);
}
// Test(sf_memsuite, reallocing_with_0_should_not_change_size, .init = sf_mem_init, .fini = sf_mem_fini) {
//     // printf("%s\n", "====================TEST 11=================");
//     void * a = sf_malloc(10);
//     sf_realloc(a,0);//REALLOCING WITH 0 SHOULD NOT CHANGE SIZE, SO FREE LIST HEAD REMAINS SAME
//     cr_assert(freelist_head == (a-8)+32);
// 		cr_assert(errno == EINVAL);
// }
Test(sf_memsuite, freeing_NULL, .init = sf_mem_init, .fini = sf_mem_fini) {
    // printf("%s\n", "====================TEST 12=================");
    void * a = sf_malloc(10);
    // sf_free(NULL);
    cr_assert(errno == 0);
    cr_assert(freelist_head == (a-8)+32);
}
// Test(sf_memsuite, splinter_merge_when_mallocing, .init = sf_mem_init, .fini = sf_mem_fini) {
//     // printf("%s\n", "====================TEST 13=================");
//     void * a = sf_malloc((4*(4096))-32);//Mallocing enough space to have splinter//We will have 16 bytes leftover
//     cr_assert(((sf_header*)(a-8))->block_size << 4 == 16384);
//     cr_assert(freelist_head == NULL);
// }
 // END OF MY TESTS
