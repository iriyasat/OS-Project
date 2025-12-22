#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <libgen.h>
#include <ctype.h>

#define NUM_THREADS 8
#define MAX_PATH 4096

typedef struct {
    char path[MAX_PATH];
} FileTask;

typedef struct {
    FileTask *tasks;
    int front;
    int rear;
    int size;
    int capacity;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} TaskQueue;

TaskQueue *queue = NULL;
pthread_t threads[NUM_THREADS];
int should_exit = 0;
pthread_mutex_t output_lock = PTHREAD_MUTEX_INITIALIZER;

TaskQueue* queue_init(int capacity) {
    TaskQueue *q = (TaskQueue *)malloc(sizeof(TaskQueue));
    q->tasks = (FileTask *)malloc(sizeof(FileTask) * capacity);
    q->front = 0;
    q->rear = -1;
    q->size = 0;
    q->capacity = capacity;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return q;
}

void queue_enqueue(TaskQueue *q, const char *path) {
    pthread_mutex_lock(&q->lock);
    
    while (q->size == q->capacity) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    
    q->rear = (q->rear + 1) % q->capacity;
    strncpy(q->tasks[q->rear].path, path, MAX_PATH - 1);
    q->tasks[q->rear].path[MAX_PATH - 1] = '\0';
    q->size++;
    
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

int queue_dequeue(TaskQueue *q, FileTask *task) {
    pthread_mutex_lock(&q->lock);
    
    while (q->size == 0 && !should_exit) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    
    if (q->size == 0) {
        pthread_mutex_unlock(&q->lock);
        return 0;
    }
    
    *task = q->tasks[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return 1;
}

void queue_free(TaskQueue *q) {
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    free(q->tasks);
    free(q);
}

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

typedef struct {
    uint32_t state[4];
    uint32_t count[2];
    unsigned char buffer[64];
} MD5_CTX;

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

#define FF(a, b, c, d, x, s, ac) { \
    (a) += F ((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
}

#define GG(a, b, c, d, x, s, ac) { \
    (a) += G ((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
}

#define HH(a, b, c, d, x, s, ac) { \
    (a) += H ((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
}

#define II(a, b, c, d, x, s, ac) { \
    (a) += I ((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
}

static void md5_transform(uint32_t state[4], const unsigned char block[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t x[16];
    
    for (int i = 0, j = 0; j < 64; i++, j += 4) {
        x[i] = ((uint32_t)block[j]) | (((uint32_t)block[j+1]) << 8) |
               (((uint32_t)block[j+2]) << 16) | (((uint32_t)block[j+3]) << 24);
    }
    
    FF(a, b, c, d, x[0], S11, 0xd76aa478);
    FF(d, a, b, c, x[1], S12, 0xe8c7b756);
    FF(c, d, a, b, x[2], S13, 0x242070db);
    FF(b, c, d, a, x[3], S14, 0xc1bdceee);
    FF(a, b, c, d, x[4], S11, 0xf57c0faf);
    FF(d, a, b, c, x[5], S12, 0x4787c62a);
    FF(c, d, a, b, x[6], S13, 0xa8304613);
    FF(b, c, d, a, x[7], S14, 0xfd469501);
    FF(a, b, c, d, x[8], S11, 0x698098d8);
    FF(d, a, b, c, x[9], S12, 0x8b44f7af);
    FF(c, d, a, b, x[10], S13, 0xffff5bb1);
    FF(b, c, d, a, x[11], S14, 0x895cd7be);
    FF(a, b, c, d, x[12], S11, 0x6b901122);
    FF(d, a, b, c, x[13], S12, 0xfd987193);
    FF(c, d, a, b, x[14], S13, 0xa679438e);
    FF(b, c, d, a, x[15], S14, 0x49b40821);
    
    GG(a, b, c, d, x[1], S21, 0xf61e2562);
    GG(d, a, b, c, x[6], S22, 0xc040b340);
    GG(c, d, a, b, x[11], S23, 0x265e5a51);
    GG(b, c, d, a, x[0], S24, 0xe9b6c7aa);
    GG(a, b, c, d, x[5], S21, 0xd62f105d);
    GG(d, a, b, c, x[10], S22, 0x2441453);
    GG(c, d, a, b, x[15], S23, 0xd8a1e681);
    GG(b, c, d, a, x[4], S24, 0xe7d3fbc8);
    GG(a, b, c, d, x[9], S21, 0x21e1cde6);
    GG(d, a, b, c, x[14], S22, 0xc33707d6);
    GG(c, d, a, b, x[3], S23, 0xf4d50d87);
    GG(b, c, d, a, x[8], S24, 0x455a14ed);
    GG(a, b, c, d, x[13], S21, 0xa9e3e905);
    GG(d, a, b, c, x[2], S22, 0xfcefa3f8);
    GG(c, d, a, b, x[7], S23, 0x676f02d9);
    GG(b, c, d, a, x[12], S24, 0x8d2a4c8a);
    
    HH(a, b, c, d, x[5], S31, 0xfffa3942);
    HH(d, a, b, c, x[8], S32, 0x8771f681);
    HH(c, d, a, b, x[11], S33, 0x6d9d6122);
    HH(b, c, d, a, x[14], S34, 0xfde5380c);
    HH(a, b, c, d, x[1], S31, 0xa4beea44);
    HH(d, a, b, c, x[4], S32, 0x4bdecfa9);
    HH(c, d, a, b, x[7], S33, 0xf6bb4b60);
    HH(b, c, d, a, x[10], S34, 0xbebfbc70);
    HH(a, b, c, d, x[13], S31, 0x289b7ec6);
    HH(d, a, b, c, x[0], S32, 0xeaa127fa);
    HH(c, d, a, b, x[3], S33, 0xd4ef3085);
    HH(b, c, d, a, x[6], S34, 0x4881d05);
    HH(a, b, c, d, x[9], S31, 0xd9d4d039);
    HH(d, a, b, c, x[12], S32, 0xe6db99e5);
    HH(c, d, a, b, x[15], S33, 0x1fa27cf8);
    HH(b, c, d, a, x[2], S34, 0xc4ac5665);
    
    II(a, b, c, d, x[0], S41, 0xf4292244);
    II(d, a, b, c, x[7], S42, 0x432aff97);
    II(c, d, a, b, x[14], S43, 0xab9423a7);
    II(b, c, d, a, x[5], S44, 0xfc93a039);
    II(a, b, c, d, x[12], S41, 0x655b59c3);
    II(d, a, b, c, x[3], S42, 0x8f0ccc92);
    II(c, d, a, b, x[10], S43, 0xffeff47d);
    II(b, c, d, a, x[1], S44, 0x85845dd1);
    II(a, b, c, d, x[8], S41, 0x6fa87e4f);
    II(d, a, b, c, x[15], S42, 0xfe2ce6e0);
    II(c, d, a, b, x[6], S43, 0xa3014314);
    II(b, c, d, a, x[13], S44, 0x4e0811a1);
    II(a, b, c, d, x[4], S41, 0xf7537e82);
    II(d, a, b, c, x[11], S42, 0xbd3af235);
    II(c, d, a, b, x[2], S43, 0x2ad7d2bb);
    II(b, c, d, a, x[9], S44, 0xeb86d391);
    
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

static void md5_init(MD5_CTX *ctx) {
    ctx->count[0] = ctx->count[1] = 0;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
}

static void md5_update(MD5_CTX *ctx, const unsigned char *input, unsigned int len) {
    unsigned int index = ctx->count[0] / 8 % 64;
    unsigned int inputLen;
    
    ctx->count[0] += len << 3;
    if (ctx->count[0] < (len << 3))
        ctx->count[1]++;
    ctx->count[1] += len >> 29;
    
    inputLen = 64 - index;
    unsigned int i = 0;
    
    if (len >= inputLen) {
        memcpy(&ctx->buffer[index], input, inputLen);
        md5_transform(ctx->state, ctx->buffer);
        
        for (i = inputLen; i + 63 < len; i += 64)
            md5_transform(ctx->state, &input[i]);
        
        index = 0;
    }
    
    memcpy(&ctx->buffer[index], &input[i], len - i);
}

static void md5_final(unsigned char digest[16], MD5_CTX *ctx) {
    unsigned char bits[8];
    for (int i = 0; i < 8; i++)
        bits[i] = (ctx->count[i >> 2] >> ((i % 4) * 8)) & 0xff;
    
    unsigned int index = ctx->count[0] / 8 % 64;
    unsigned int padLen = (index < 56) ? (56 - index) : (120 - index);
    unsigned char padding[64] = {0x80};
    
    md5_update(ctx, padding, padLen);
    md5_update(ctx, bits, 8);
    
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            digest[i*4+j] = (ctx->state[i] >> (j * 8)) & 0xff;
}

char* calculate_md5(const char *filepath) {
    unsigned char digest[16];
    char *md5str = (char *)malloc(33);
    
    FILE *f = fopen(filepath, "rb");
    if (f == NULL) {
        snprintf(md5str, 33, "00000000000000000000000000000000");
        return md5str;
    }
    
    MD5_CTX ctx;
    md5_init(&ctx);
    
    unsigned char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), f)) != 0) {
        md5_update(&ctx, buffer, bytes);
    }
    
    fclose(f);
    md5_final(digest, &ctx);
    
    for (int i = 0; i < 16; i++) {
        sprintf(md5str + (i * 2), "%02x", (unsigned int)digest[i]);
    }
    md5str[32] = '\0';
    
    return md5str;
}

void add_files_from_directory(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        return;
    }
    
    struct dirent *entry;
    char full_path[MAX_PATH];
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, MAX_PATH, "%s/%s", dir_path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == -1) {
            continue;
        }
        
        if (S_ISREG(st.st_mode)) {
            queue_enqueue(queue, full_path);
        } else if (S_ISDIR(st.st_mode)) {
            add_files_from_directory(full_path);
        }
    }
    
    closedir(dir);
}

void* worker_thread(__attribute__((unused)) void *arg) {
    FileTask task;
    
    while (queue_dequeue(queue, &task)) {
        char *md5 = calculate_md5(task.path);
        
        char tmp[MAX_PATH];
        strncpy(tmp, task.path, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';
        char *base = basename(tmp);

        for (char *p = md5; *p; ++p) {
            *p = (char)toupper((unsigned char)*p);
        }

        pthread_mutex_lock(&output_lock);
        printf("%s %s\n", base, md5);
        fflush(stdout);
        pthread_mutex_unlock(&output_lock);
        
        free(md5);
    }
    
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        char prog_buf[MAX_PATH];
        strncpy(prog_buf, argv[0] ? argv[0] : "md5hash", sizeof(prog_buf)-1);
        prog_buf[sizeof(prog_buf)-1] = '\0';
        char *prog = basename(prog_buf);
        fprintf(stderr, "USAGE: %s <directory/file> [more directories/files]\n", prog);
        return 1;
    }
    
    queue = queue_init(1000);

    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, worker_thread, NULL) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            return 1;
        }
    }

    for (int i = 1; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) == -1) {
            fprintf(stderr, "Cannot access: %s\n", argv[i]);
            continue;
        }
        
        if (S_ISREG(st.st_mode)) {
            queue_enqueue(queue, argv[i]);
        } else if (S_ISDIR(st.st_mode)) {
            add_files_from_directory(argv[i]);
        }
    }

    should_exit = 1;
    pthread_cond_broadcast(&queue->not_empty);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Failed to join thread %d\n", i);
            return 1;
        }
    }
    
    queue_free(queue);
    pthread_mutex_destroy(&output_lock);
    
    return 0;
}
