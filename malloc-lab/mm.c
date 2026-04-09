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

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_init - initialize the malloc package.
 */
// 메모리를 초기화 하는 로직: 한 테스트 시나리오 종료 시 발동
int mm_init(void)
{
    printf("\nmm_init: 모든 메모리가 초기화 됩니다.\n\n");
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
// 메모리를 할당하는 로직: 현재 기본적인 것만 구현되어 있음
// 여기서 이해한 내용: malloc은 실제 모두 채워넣는 것이 아닌, header(명표)만 만들어 놓는다.
void *mm_malloc(size_t size) // void와 void *는 다름 .. size_t => 메모리 크기를 표현하는 타입
{
    int newsize = ALIGN(size + SIZE_T_SIZE);  // 8바이트 규격에 맞게 size 설정
    printf("mm_malloc: 점유 공간 | 실제 메모리 양: %8d | %8d  ", newsize, (int)size);
    
    void *p = mem_sbrk(newsize);  // p라는 변수에 "주소 숫자"를 저장함.
    if (p == (void *)-1){
        printf("=> 공간이 부족합니다(실패)! \n");  // 에러 결과값(-1)에 따른 방어 코드
        return NULL;
    }   
    else
    {
        *(size_t *)p = size; // p가 가리키는 메모리 그 자체에 "블록 크기(size)"를 저장함. 
                             // (선언* 와 이후*는 다름)
                             // 만약 (size_t)p => 강제로 양수로 형변환 됨 
                             // (size_t *) => 해당 p(주소값)은 size_t를 기록한 메모리의 주소값이라고 가정
                             // * => 그리고 그 주소값으로 가
                             // 따라서: p가 가리키는 실제 주소값에 size라는 양의 정수가 header(8바이트)에 입력됨
        printf(" ===>   공간을 할당하였습니다! .. 명표(header) 위치: %p \n", p);
        return (void *)((char *)p + SIZE_T_SIZE); // 다시 (char *)로 형 변환 
                                                  // size_t(8바이트) => char(1바이트) 로 형 변환
                                                  // 8 + 1 = 9번째 주소값을 반환: 데이터 입력이 시작되는 주소값
                                                  // void * => 타입명 다시 변경(무엇이든 다시 가능)                                               
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
// 메모리를 해제하는 로직
void mm_free(void *ptr)
{
    printf("mm_free: 공간 해제를 요청합니다.\n");
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
// 메모리 재할당 로직: 축소 and 확장 모두 가능하도록 지원해야 함.
// 동적 메모리 할당 => 국소적 관리를 이용할 것
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    printf("mm_realloc: 공간 재할당 시도 ");
    if (newptr == NULL){
        printf("=> 재할당 공간이 부족합니다(실패)! \n");
        return NULL;
    }
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    printf("=> newptr <-> oldptr 간 재조정이 일어났습니다! \n");
    mm_free(oldptr);
    return newptr;
}