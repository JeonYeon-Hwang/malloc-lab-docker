/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define LISTLIMIT 15
// #define DEBUG

#ifdef DEBUG
    #define dbg_printf(...) printf(__VA_ARGS__)
#else
    #define dbg_printf(...)
#endif

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define GET_SIZE(p) (*(size_t *)(p) & ~0x7) // 해당 주소의 8바이트 값 읽어서 size 도출
#define GET_ALLOC(p) (*(size_t *)(p) & 0x1) // 해당 주소의 1바이트 값 읽어서 할당유무 확인
#define HDRP(bp) ((char *)bp - SIZE_T_SIZE) // payload 주소값으로 header 찾기
#define FTRP(bp) ((char *)bp + GET_SIZE(HDRP(bp)) - (2 * SIZE_T_SIZE)) // payload 주소값으로 footer 찾기

// 얘들은 일단 복붙
#define WSIZE (SIZE_T_SIZE)
#define DSIZE (2 * SIZE_T_SIZE)
#define PACK(size, alloc) ((size) | (alloc))
#define PUT(p, val) (*(size_t *)(p) = (val))
#define NEXT_BLKP(bp) (((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) (((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))) 

#define PREV_A_TO_B(bp1, bp2) (*(void **)(bp1) = (bp2)) // 현 payload(prev)에 bp2 주소값 넣기
#define NEXT_A_TO_B(bp1, bp2) (*(void **)((char *)(bp1) + WSIZE) = (bp2)) // 현 payload + 8바이트 자리(next)에 bp2 주소값 넣기
// #define GET_ROOT(class) (*(void **)((char *)(heap_listp) + (WSIZE * class))) // 해당 리스트의 root 주소값

static char *heap_listp; // heap의 시작 주소 => 1바이트 단위 계산을 위해 char로 설정
// static char *rover; // next fit 용 주소
static char *top_free; // 이전에 해제한 메모리의 payload 주소
static char *segregation_list[LISTLIMIT]; // seglist 루트 주소값 저장 리스트

static void *find_fit(size_t put_size); // implicit 구현: 사이 탐색 함수
static void *coalesce(void *bp); // 인접 free 공간 병합 함수
static void *extend_heap(size_t new_size); // 힙을 확장하는 함수

static void place(void *bp, size_t new_size); // 기존 free 공간에 할당/분할 함수
static void insert_node(void *bp, size_t size); // 노드 삽입하기
static void delete_node(void *bp); // 노드 삭제하기

/*
 * mm_init - initialize the malloc package.
 */
// 메모리를 초기화 하는 로직: 한 테스트 시나리오 종료 시 발동
int mm_init(void)
{
    char *p = mem_sbrk(6 * WSIZE); // 48바이트 확보
    if (p == (void *)-1) return -1;

    int list;

    for(list = 0; list < LISTLIMIT; list++){
        segregation_list[list] = NULL; // seg_list 초기화
    }

    
    *(size_t *)p = 0; // 패딩: 모두 0으로 채우기
    
    *(size_t *)(p + WSIZE) = ((2 * DSIZE) | 1); // 프롤로그 헤더 => size가 8, 할당상태
    *(void **)(p + (2 * WSIZE)) = NULL; // 프롤로그 prev 포인터 => NULL 상태
    *(void **)(p + (3 * WSIZE)) = NULL; // 프롤로그 next 포인터 => NULL 상태
    *(size_t *)(p + (4 * WSIZE)) = ((2 * DSIZE) | 1); // 프롤로그 푸터 => size: 8, 할당상태
    
    *(size_t *)(p + (5 * WSIZE)) = (0 | 1); // 에필로그 헤더 => size가 0, 할당상태

    heap_listp = p + (2 * WSIZE); // 힙 시작 주소 설정: 현 payload 주소
    top_free = p + (2 * WSIZE); // 바로 이전에 해제한 메모리 주소: 현 payload 주소
    // rover = heap_listp; // 초기 rover는 heap 
    
    dbg_printf("\nmm_init:  heap_strt: %p (0 bytes) \n\n", heap_listp);
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
// 메모리를 할당하는 로직: 현재 기본적인 것만 구현되어 있음
void *mm_malloc(size_t size) // void와 void *는 다름 .. size_t => 메모리 크기를 표현하는 타입
{
    int newsize = ALIGN(size + (2 * SIZE_T_SIZE));  //
    dbg_printf("mm_malloc 실행 => %8d %8d", newsize, (int)size);
    
    void *internal_p = find_fit(newsize);
    if(internal_p != (void *)-1){
        place(internal_p, newsize);
              
        dbg_printf("  =>  내부 찾음   header: %p (%ld bytes)\n", HDRP(internal_p), (char *)HDRP(internal_p) - (char *)heap_listp); 
        return(void *)internal_p; // 사용자가 할당할 payload 주소를 반환
    }
    
    void *bp = extend_heap(newsize); // 힙 확장
    place(bp, newsize); // 배치하고
    return (void *)bp; // payload 반환
}

/*
 * mm_free - Freeing a block does nothing.
 */
// 메모리를 해제하는 로직
// header와 footer 모두 size_t 형식의 데이터 양을 명시한다
void mm_free(void *ptr) // ptr 주소 = payload 주소
{
    if(ptr == NULL) return;

    size_t size = GET_SIZE(HDRP(ptr)); // payload 기반 size 추출
    PUT(HDRP(ptr), PACK(size, 0)); //payload 기반 헤더 할당해제
    PUT(FTRP(ptr), PACK(size, 0)); //payload 기반 푸터 할당해제
    
    dbg_printf("매모리 해제    =>  header: %8p   size: %d bytes\n", HDRP(ptr), (int)(size));
    
    coalesce(ptr); // 병합하기
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
// 메모리 재할당 로직: 축소 and 확장 모두 가능하도록 지원해야 함.
void *mm_realloc(void *ptr, size_t size) // 기본 구현
{
    if (ptr == NULL) return mm_malloc(size); // 기본 반환
    if (size == 0){
        mm_free(ptr); 
        return NULL;
    } 
    
    size_t before_size = GET_SIZE(HDRP(ptr)); // 해제되는 곳의 현 size
    size_t input_size = ALIGN(size + (2 * SIZE_T_SIZE)); // 갱신되어야 할 새 size
    char *next_bp = NEXT_BLKP(ptr); // 다음 블록의 payload

    if(input_size > before_size){ // 새 size가 더 큰 상황
        if(GET_ALLOC(HDRP(next_bp)) == 0 && before_size + GET_SIZE(HDRP(next_bp)) >= input_size ){ // 뒷 블록이 빈 & 빈 공간에 여분 채우기 가능
            // disconnect(next_bp); // 인접 free 블럭을 연결리스트에서 삭제
            delete_node(next_bp);
            
            size_t merge_size = before_size + GET_SIZE(HDRP(next_bp)); // 합칠 경우 총 size
            size_t extra_size = merge_size - input_size;

            if(extra_size >= (2 * DSIZE)){
                PUT(HDRP(ptr), PACK(input_size, 1));
                PUT(FTRP(ptr), PACK(input_size, 1));

                void *remain_bp = (char *)ptr + input_size; // 여분 size의 시작 payload
                
                PUT(HDRP(remain_bp), PACK(extra_size, 0)); 
                PUT(FTRP(remain_bp), PACK(extra_size, 0));

                // new_connect(remain_bp);
                insert_node(remain_bp, extra_size);
            }
            else{
                PUT(HDRP(ptr), PACK(merge_size, 1));
                PUT(FTRP(ptr), PACK(merge_size, 1));
            }

            return ptr;
        }
        else{
            void *newptr = mm_malloc(size); // mm_malloc 호출하여 사용
            if (newptr == NULL) return NULL;

            size_t copySize = GET_SIZE(HDRP(ptr)) - 2 * SIZE_T_SIZE;
            if (size < copySize) copySize = size; 
            
            memcpy(newptr, ptr, copySize); // 잘 모르지만 .. 그대로 데이터 복사함
            mm_free(ptr);

            return newptr;
        }

    }
    else if(input_size < before_size){ // 새 size가 더 작은 상황
        size_t left_size = before_size - input_size;

        if(left_size >= (2 * DSIZE)){ // 만약 free할 여분의 공간이 충분히 있다면?
            PUT(HDRP(ptr), PACK(input_size, 1));
            PUT(FTRP(ptr), PACK(input_size, 1));

            void *remain_bp = (char *)ptr + input_size;

            PUT(HDRP(remain_bp), PACK(left_size, 0)); // 남은 여분 설정
            PUT(FTRP(remain_bp), PACK(left_size, 0));

            coalesce(remain_bp); // 혹시나 여분이 있는지 병합처리
        }

        return ptr;
    }
    else{ // 동일한 경우
        return ptr;
    }  
}




// seglist 구현: index찾기, root => 끝까지
static void *find_fit(size_t put_size){  
    int list = 0;
    size_t block_size = put_size;
    size_t class_size = put_size;

    while((list < LISTLIMIT - 1) && (class_size > 1)){ // 마찬가지로, index를 구함
        class_size >>= 1;
        list++;
    }

    for(; list < LISTLIMIT; list++){ // 찾지 못했을 경우, 다음 index로 넘어감
        void *search_bp = segregation_list[list];

        while(search_bp != NULL){
            if(GET_SIZE(HDRP(search_bp)) >= block_size){
                return search_bp;
            }

            search_bp = *(void **)((char *)search_bp + WSIZE);
        }
    }
    
    return (void *)-1; // 없을 경우 -1 반환
}




// 병합 함수: mm_free & extend_heap 이벤트에서 호출 => 4가지 경우의 수
static void *coalesce(void *bp){ 
    char *prev_bp = (char *)bp - GET_SIZE((char *)(bp) - (2 * SIZE_T_SIZE)); // 이전 블록의 payload
    char *next_bp = (char *)bp + GET_SIZE((char *)(bp) - (SIZE_T_SIZE)); // 다음 블록의 payload

    size_t prev_alloc = GET_ALLOC(FTRP(prev_bp)); // 현 payload 기준 => 푸터로 이동하여 할당유무 확인
    size_t next_alloc = GET_ALLOC(HDRP(next_bp)); // 현 payload 기준 => 헤더로 이동하여 할당유무 확인
    size_t size = GET_SIZE(HDRP(bp)); // 현 payload 기준 => 블록의 size 추출

    if(prev_alloc && next_alloc){ // 앞 뒤 블록이 모두 할당되어 있다면?
        // new_connect(bp); // 스택에 free블록 하나 쌓기
        insert_node(bp, size);
        return bp; // 변화 없음
    }
    else if(prev_alloc && !next_alloc){ // 뒤 블록만 free라면?
        delete_node(next_bp);

        size += GET_SIZE(HDRP(next_bp)); // 뒤 블록의 size를 더함
        *(size_t *)HDRP(bp) = (size | 0); // 현 블록 헤더 size 확장 
        *(size_t *)FTRP(bp) = (size | 0); // 현 블록 푸터 size 확장 

        // disconnect(next_bp); // 뒤 free 없애기
        // new_connect(bp); // 합쳐진 free블록을 쌓기
        insert_node(bp, size);
    }
    else if(!prev_alloc && next_alloc){ // 앞 블록만 free라면?
        delete_node(prev_bp);
        
        size += GET_SIZE(HDRP(prev_bp)); // 앞 블록의 size를 더함
        *(size_t *)HDRP(prev_bp) = (size | 0); // 앞 블록 헤더 size 확장
        *(size_t *)FTRP(prev_bp) = (size | 0); // 현 블록 푸터 size 확장
        
        // disconnect(prev_bp); // 앞 free블록을 일단 없앰
        bp = prev_bp; // bp를 앞 블록 payload로 이동
        // new_connect(bp); // 합쳐진 free블록으로 쌓기
        insert_node(bp, size);
    }
    else if(!prev_alloc && !next_alloc){ // 앞 뒤 블록 모두 free라면?
        delete_node(prev_bp);
        delete_node(next_bp);
        
        size += (GET_SIZE(HDRP(next_bp)) + GET_SIZE(HDRP(prev_bp))); // 앞 뒤 블록의 size를 모두 더함
        *(size_t *)HDRP(prev_bp) = (size | 0); // 앞 블록의 헤더 size 확장
        *(size_t *)FTRP(prev_bp) = (size | 0); // 뒤 블록의 푸터 size 확장
        
        // disconnect(prev_bp); // 앞 free 없애기
        // disconnect(next_bp); // 뒤 free 없애기
        bp = prev_bp; // bp를 앞 블록 payload로 이동
        // new_connect(bp); // 앞뒤로 합쳐진 큰 free블록을 쌓기
        insert_node(bp, size);
    }
    dbg_printf("병합됨         =>  header: %p(%ld bytes)   size:%d bytes\n", HDRP(bp), HDRP(bp) - (char *)heap_listp, (int)(size));
    // rover = bp;
    return bp;
}



// 힙 확장 함수
static void *extend_heap(size_t new_size){
    void *bp = mem_sbrk(new_size); // mem_sbrk: 힙 확장 범위를 결정 & 기존 payload 반환

    if (bp == (void *)-1){ // 메모리가 초과했다면?
        dbg_printf("=> 공간 부족!  \n");  
        return NULL;
    } 
    else{
        PUT(HDRP(bp), PACK(new_size, 0)); // 새 헤더, 비할당
        PUT(FTRP(bp), PACK(new_size, 0)); // 새 푸터, 비할당
        PUT(FTRP(bp) + SIZE_T_SIZE, PACK(0, 1)); // 에필로그 이사

        bp = coalesce(bp);

        dbg_printf("  =>  힙 팽창함   header: %p (%ld bytes)\n", HDRP(bp), HDRP(bp) - (char *)heap_listp);
        return (void *)((char *)bp);
    }
}




// 배치 함수: 남은 여분의 free_size를 기준으로 배치를 결정한다
static void place(void *bp, size_t new_size){ // 받는 인자: 할당 payload & 할당할 size
    size_t free_size = GET_SIZE(HDRP(bp)); // 먼저 free_size를 구한다
    int free_amount = free_size - new_size; // 여분이 될 free_size

    // disconnect(bp); // free 스택에서 연결 해제
    delete_node(bp);

    if(free_amount >= (2 * DSIZE)){ // 여분의 공간이 32바이트 이상이라면?
        PUT(HDRP(bp), PACK(new_size, 1)); // 헤더 new_size로 갱신
        PUT(FTRP(bp), PACK(new_size, 1)); // 푸터 new_size로 갱신

        void *next_bp = (char *)bp + new_size; // 여분이 될 free_size의 payload

        PUT(HDRP(next_bp), PACK(free_amount, 0)); // 여분 헤더 생성
        PUT(FTRP(next_bp), PACK(free_amount, 0)); // 여분 푸터 생성
        
        // new_connect(next_bp); // 일단 새로 생긴 free이니..
        insert_node(next_bp, free_amount);
    }
    else{ // 여분의 공간이 16 바이트 미만이라면? => 내부 단편화 감수
        PUT(HDRP(bp), PACK(free_size, 1));
        PUT(FTRP(bp), PACK(free_size, 1));
    }
}




// seglist: 노드 삽입하기(payload, 메모리 size)
static void insert_node(void *bp, size_t size){
    int list = 0;
    size_t class_size = size; // class 계산용

    while((list < LISTLIMIT - 1) && (class_size > 1)){
        class_size >>= 1; // 5: 63- ,6: 127-, 7: 255-, .... , 15: 65535-, 16: 65536>
        list++; // 인덱스 구해짐
    }

    NEXT_A_TO_B(bp, segregation_list[list]); // 최신 블록으로 갱신
    PREV_A_TO_B(bp, NULL);

    if(segregation_list[list] != NULL){
        PREV_A_TO_B(segregation_list[list], bp); // 이전 블록의 prev 갱신
    }

    segregation_list[list] = bp; // 최신값 => root로 갱신
}




// seglist: 노드 삭제하기(payload)
static void delete_node(void *bp){
    void *prev_bp = *(void **)bp; // 앞 블록 포인터
    void *next_bp = *(void **)((char *)bp + WSIZE); // 뒷 블록 포인터
    
    int list = 0;
    size_t block_size = GET_SIZE(HDRP(bp));
    size_t class_size = block_size;

    while((list < LISTLIMIT - 1) && (class_size > 1)){ // 마찬가지로, index를 구함
        class_size >>= 1;
        list++;
    }

    if(prev_bp == NULL && next_bp == NULL){ // 앞 뒤 없음
        segregation_list[list] = NULL; 
    }
    else if(prev_bp == NULL && next_bp != NULL){ // 앞 없음
        segregation_list[list] = next_bp; 
        PREV_A_TO_B(next_bp, NULL); // 뒤로 가서, 앞 칸 없음 표시
    }
    else if(prev_bp != NULL && next_bp == NULL){ // 뒤 없음
        NEXT_A_TO_B(prev_bp, NULL);  // 앞으로 가서, 뒷 칸 없음 표시
    }
    else if(prev_bp != NULL && next_bp != NULL){ // 앞 뒤 있음
        NEXT_A_TO_B(prev_bp, next_bp); // 배선 정리
        PREV_A_TO_B(next_bp, prev_bp); 
    }


    PREV_A_TO_B(bp, NULL); // 현 prev 없앰
    NEXT_A_TO_B(bp, NULL); // 현 next 없앰
}



// short1-bal.rep 시나리오: Perf index = 40 (util) + 40 (thru) = 80/100
// short2-bal.rep 시나리오: Perf index = 60 (util) + 40 (thru) = 100/100

// implicit 통합 테스트: Perf index = 46 (util) + 16 (thru) = 62/100
// explicit 통합 테스트: Perf index = 44 (util) + 40 (thru) = 84/100

// realloc 수정 후: Perf index = 45 (util) + 40 (thru) = 85/100
// seglist 통합 테스트: Perf index = 47 (util) + 40 (thru) = 87/100