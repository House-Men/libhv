#include "hbase.h"

#ifdef OS_DARWIN
#include <mach-o/dyld.h> // for _NSGetExecutablePath
#endif

#include "hatomic.h"

#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif

static hatomic_t s_alloc_cnt = HATOMIC_VAR_INIT(0);
static hatomic_t s_free_cnt = HATOMIC_VAR_INIT(0);

long hv_alloc_cnt() {
    return s_alloc_cnt;
}

long hv_free_cnt() {
    return s_free_cnt;
}

void* hv_malloc(size_t size) {
    hatomic_inc(&s_alloc_cnt);
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "malloc failed!\n");
        exit(-1);
    }
    return ptr;
}

void* hv_realloc(void* oldptr, size_t newsize, size_t oldsize) {
    hatomic_inc(&s_alloc_cnt);
    if (oldptr) hatomic_inc(&s_free_cnt);
    void* ptr = realloc(oldptr, newsize);
    if (!ptr) {
        fprintf(stderr, "realloc failed!\n");
        exit(-1);
    }
    if (newsize > oldsize) {
        memset((char*)ptr + oldsize, 0, newsize - oldsize);
    }
    return ptr;
}

void* hv_calloc(size_t nmemb, size_t size) {
    hatomic_inc(&s_alloc_cnt);
    void* ptr =  calloc(nmemb, size);
    if (!ptr) {
        fprintf(stderr, "calloc failed!\n");
        exit(-1);
    }
    return ptr;
}

void* hv_zalloc(size_t size) {
    hatomic_inc(&s_alloc_cnt);
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "malloc failed!\n");
        exit(-1);
    }
    memset(ptr, 0, size);
    return ptr;
}

void hv_free(void* ptr) {
    if (ptr) {
        free(ptr);
        ptr = NULL;
        hatomic_inc(&s_free_cnt);
    }
}

char* hv_strupper(char* str) {
    char* p = str;
    while (*p != '\0') {
        if (*p >= 'a' && *p <= 'z') {
            *p &= ~0x20;
        }
        ++p;
    }
    return str;
}

char* hv_strlower(char* str) {
    char* p = str;
    while (*p != '\0') {
        if (*p >= 'A' && *p <= 'Z') {
            *p |= 0x20;
        }
        ++p;
    }
    return str;
}

char* hv_strreverse(char* str) {
    if (str == NULL) return NULL;
    char* b = str;
    char* e = str;
    while(*e) {++e;}
    --e;
    char tmp;
    while (e > b) {
        tmp = *e;
        *e = *b;
        *b = tmp;
        --e;
        ++b;
    }
    return str;
}

// n = sizeof(dest_buf)
char* hv_strncpy(char* dest, const char* src, size_t n) {
    assert(dest != NULL && src != NULL);
    char* ret = dest;
    while (*src != '\0' && --n > 0) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return ret;
}

// n = sizeof(dest_buf)
char* hv_strncat(char* dest, const char* src, size_t n) {
    assert(dest != NULL && src != NULL);
    char* ret = dest;
    while (*dest) {++dest;--n;}
    while (*src != '\0' && --n > 0) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return ret;
}

bool hv_strstartswith(const char* str, const char* start) {
    assert(str != NULL && start != NULL);
    while (*str && *start && *str == *start) {
        ++str;
        ++start;
    }
    return *start == '\0';
}

bool hv_strendswith(const char* str, const char* end) {
    assert(str != NULL && end != NULL);
    int len1 = 0;
    int len2 = 0;
    while (*str) {++str; ++len1;}
    while (*end) {++end; ++len2;}
    if (len1 < len2) return false;
    while (len2-- > 0) {
        --str;
        --end;
        if (*str != *end) {
            return false;
        }
    }
    return true;
}

bool hv_strcontains(const char* str, const char* sub) {
    assert(str != NULL && sub != NULL);
    return strstr(str, sub) != NULL;
}

bool hv_wildcard_match(const char* str, const char* pattern) {
    assert(str != NULL && pattern != NULL);
    bool match = false;
    while (*str && *pattern) {
        if (*pattern == '*') {
            match = hv_strendswith(str, pattern + 1);
            break;
        } else if (*str != *pattern) {
            match = false;
            break;
        } else {
            ++str;
            ++pattern;
        }
    }
    return match ? match : (*str == '\0' && *pattern == '\0');
}

char* hv_strnchr(const char* s, char c, size_t n) {
    assert(s != NULL);
    const char* p = s;
    while (*p != '\0' && n-- > 0) {
        if (*p == c) return (char*)p;
        ++p;
    }
    return NULL;
}

char* hv_strnrchr(const char* s, char c, size_t n) {
    assert(s != NULL);
    const char* p = s;
    const char* last = NULL;
    while (*p != '\0' && n-- > 0) {
        if (*p == c) last = p;
        ++p;
    }
    return (char*)last;
}

char* hv_strrchr_dir(const char* filepath) {
    char* p = (char*)filepath;
    while (*p) ++p;
    while (--p >= filepath) {
#ifdef OS_WIN
        if (*p == '/' || *p == '\\')
#else
        if (*p == '/')
#endif
            return p;
    }
    return NULL;
}

const char* hv_basename(const char* filepath) {
    const char* pos = hv_strrchr_dir(filepath);
    return pos ? pos+1 : filepath;
}

const char* hv_suffixname(const char* filename) {
    const char* pos = hv_strrchr_dot(filename);
    return pos ? pos+1 : "";
}

int hv_mkdir_p(const char* dir) {
    if (access(dir, 0) == 0) {
        return EEXIST;
    }
    char tmp[MAX_PATH] = {0};
    hv_strncpy(tmp, dir, sizeof(tmp));
    char* p = tmp;
    char delim = '/';
    while (*p) {
#ifdef OS_WIN
        if (*p == '/' || *p == '\\') {
            delim = *p;
#else
        if (*p == '/') {
#endif
            *p = '\0';
            hv_mkdir(tmp);
            *p = delim;
        }
        ++p;
    }
    if (hv_mkdir(tmp) != 0) {
        return EPERM;
    }
    return 0;
}

int hv_rmdir_p(const char* dir) {
    if (access(dir, 0) != 0) {
        return ENOENT;
    }
    if (rmdir(dir) != 0) {
        return EPERM;
    }
    char tmp[MAX_PATH] = {0};
    hv_strncpy(tmp, dir, sizeof(tmp));
    char* p = tmp;
    while (*p) ++p;
    while (--p >= tmp) {
#ifdef OS_WIN
        if (*p == '/' || *p == '\\') {
#else
        if (*p == '/') {
#endif
            *p = '\0';
            if (rmdir(tmp) != 0) {
                return 0;
            }
        }
    }
    return 0;
}

bool hv_exists(const char* path) {
    return access(path, 0) == 0;
}

bool hv_isdir(const char* path) {
    if (access(path, 0) != 0) return false;
    struct stat st;
    memset(&st, 0, sizeof(st));
    stat(path, &st);
    return S_ISDIR(st.st_mode);
}

bool hv_isfile(const char* path) {
    if (access(path, 0) != 0) return false;
    struct stat st;
    memset(&st, 0, sizeof(st));
    stat(path, &st);
    return S_ISREG(st.st_mode);
}

bool hv_islink(const char* path) {
#ifdef OS_WIN
    return hv_isdir(path) && (GetFileAttributes(path) & FILE_ATTRIBUTE_REPARSE_POINT);
#else
    if (access(path, 0) != 0) return false;
    struct stat st;
    memset(&st, 0, sizeof(st));
    lstat(path, &st);
    return S_ISLNK(st.st_mode);
#endif
}

size_t hv_filesize(const char* filepath) {
    struct stat st;
    memset(&st, 0, sizeof(st));
    stat(filepath, &st);
    return st.st_size;
}

char* get_executable_path(char* buf, int size) {
#ifdef OS_WIN
    GetModuleFileName(NULL, buf, size);
#elif defined(OS_LINUX)
    if (readlink("/proc/self/exe", buf, size) == -1) {
        return NULL;
    }
#elif defined(OS_DARWIN)
    _NSGetExecutablePath(buf, (uint32_t*)&size);
#endif
    return buf;
}

char* get_executable_dir(char* buf, int size) {
    char filepath[MAX_PATH] = {0};
    get_executable_path(filepath, sizeof(filepath));
    char* pos = hv_strrchr_dir(filepath);
    if (pos) {
        *pos = '\0';
        strncpy(buf, filepath, size);
    }
    return buf;
}

char* get_executable_file(char* buf, int size) {
    char filepath[MAX_PATH] = {0};
    get_executable_path(filepath, sizeof(filepath));
    char* pos = hv_strrchr_dir(filepath);
    if (pos) {
        strncpy(buf, pos+1, size);
    }
    return buf;
}

char* get_run_dir(char* buf, int size) {
    return getcwd(buf, size);
}

int hv_rand(int min, int max) {
    static int s_seed = 0;
    assert(max > min);

    if (s_seed == 0) {
        s_seed = time(NULL);
        srand(s_seed);
    }

    int _rand = rand();
    _rand = min + (int) ((double) ((double) (max) - (min) + 1.0) * ((_rand) / ((RAND_MAX) + 1.0)));
    return _rand;
}

char* hv_random_string(char *buf, int len) {
    static char s_characters[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U',
        'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    };
    int i = 0;
    for (; i < len; i++) {
        buf[i] = s_characters[hv_rand(0, sizeof(s_characters) - 1)];
    }
    buf[i] = '\0';
    return buf;
}

bool hv_getboolean(const char* str) {
    if (str == NULL) return false;
    int len = strlen(str);
    if (len == 0) return false;
    switch (len) {
    case 1: return *str == '1' || *str == 'y' || *str == 'Y';
    case 2: return stricmp(str, "on") == 0;
    case 3: return stricmp(str, "yes") == 0;
    case 4: return stricmp(str, "true") == 0;
    case 6: return stricmp(str, "enable") == 0;
    default: return false;
    }
}

size_t hv_parse_size(const char* str) {
    size_t size = 0, n = 0;
    const char* p = str;
    char c;
    while ((c = *p) != '\0') {
        if (c >= '0' && c <= '9') {
            n = n * 10 + c - '0';
        } else {
            switch (c) {
            case 'K': case 'k': n <<= 10; break;
            case 'M': case 'm': n <<= 20; break;
            case 'G': case 'g': n <<= 30; break;
            case 'T': case 't': if(sizeof(size_t) > 5) n <<= 40; break;
            default:                      break;
            }
            size += n;
            n = 0;
        }
        ++p;
    }
    return size + n;
}

time_t hv_parse_time(const char* str) {
    time_t time = 0, n = 0;
    const char* p = str;
    char c;
    while ((c = *p) != '\0') {
        if (c >= '0' && c <= '9') {
            n = n * 10 + c - '0';
        } else {
            switch (c) {
            case 's':                           break;
            case 'm': n *= 60;                  break;
            case 'h': n *= 60 * 60;             break;
            case 'd': n *= 24 * 60 * 60;        break;
            case 'w': n *= 7 * 24 * 60 * 60;    break;
            default:                            break;
            }
            time += n;
            n = 0;
        }
        ++p;
    }
    return time + n;
}

int hv_parse_url(hurl_t* stURL, const char* strURL) {
    if (stURL == NULL || strURL == NULL) return -1;
    memset(stURL, 0, sizeof(hurl_t));
    const char* begin = strURL;
    const char* end = strURL;
    while (*end != '\0') ++end;
    if (end - begin > 65535) return -2;
    // scheme://
    const char* sp = strURL;
    const char* ep = strstr(sp, "://");
    if (ep) {
        // stURL->fields[HV_URL_SCHEME].off = sp - begin;
        stURL->fields[HV_URL_SCHEME].len = ep - sp;
        sp = ep + 3;
    }
    // user:pswd@host:port
    ep = strchr(sp, '/');
    if (ep == NULL) ep = end;
    const char* user = sp;
    const char* host = sp;
    const char* pos = hv_strnchr(sp, '@', ep - sp);
    if (pos) {
        // user:pswd
        const char* pswd = hv_strnchr(user, ':', pos - user);
        if (pswd) {
            stURL->fields[HV_URL_PASSWORD].off = pswd + 1 - begin;
            stURL->fields[HV_URL_PASSWORD].len = pos - pswd - 1;
        } else {
            pswd = pos;
        }
        stURL->fields[HV_URL_USERNAME].off = user - begin;
        stURL->fields[HV_URL_USERNAME].len = pswd - user;
        // @
        host = pos + 1;
    }
    // host:port or ipv4:port or [ipv6]:port
    const char* hostend = host;
    if (*host == '[') {
        pos = hv_strnchr(host, ']', ep - host);
        if (pos) {
            // ipv6
            host++;
            hostend = pos;
            stURL->fields[HV_URL_HOST].off = host - begin;
            stURL->fields[HV_URL_HOST].len = hostend - host;
        }
    }
    const char* port = hv_strnchr(hostend, ':', ep - hostend);
    if (port) {
        stURL->fields[HV_URL_PORT].off = port + 1 - begin;
        stURL->fields[HV_URL_PORT].len = ep - port - 1;
        // atoi
        for (unsigned short i = 1; i <= stURL->fields[HV_URL_PORT].len; ++i) {
            stURL->port = stURL->port * 10 + (port[i] - '0');
        }
    } else {
        port = ep;
        // set default port
        stURL->port = 80;
        if (stURL->fields[HV_URL_SCHEME].len > 0) {
            if (strncmp(strURL, "https://", 8) == 0) {
                stURL->port = 443;
            }
        }
    }
    if (stURL->fields[HV_URL_HOST].len == 0) {
        stURL->fields[HV_URL_HOST].off = host - begin;
        stURL->fields[HV_URL_HOST].len = port - host;
    }
    if (ep == end) return 0;
    // /path
    sp = ep;
    const char* query = strchr(sp, '?');
    const char* fragment = strchr(sp, '#');
    if (query && fragment) ep = MIN(query, fragment);
    else if (query == NULL && fragment) ep = fragment;
    else if (query == NULL) ep = end;
    else ep = query;
    stURL->fields[HV_URL_PATH].off = sp - begin;
    stURL->fields[HV_URL_PATH].len = ep - sp;
    if (ep == end) return 0;
    if (ep != fragment) {
        // ?query
        sp = ep + 1;
        ep = fragment;
        if (ep == NULL) ep = end;
        stURL->fields[HV_URL_QUERY].off = sp - begin;
        stURL->fields[HV_URL_QUERY].len = ep - sp;
        if (ep == end) return 0;
    }
    // #fragment
    sp = ep + 1;
    ep = end;
    stURL->fields[HV_URL_FRAGMENT].off = sp - begin;
    stURL->fields[HV_URL_FRAGMENT].len = ep - sp;
    return 0;
}

int hv_normalize_path(char *path) {
    if (*path != '/') return 0;
    int pos = 1;
#ifdef OS_WIN
    int sum = 0;
#endif
    for (int i = 1; path[i] != '\0'; ++i) {
        switch (path[i]) {
            case '\\':
            case '/':
                if (path[pos - 1] != '/') path[pos++] = '/';
                break;

            case '.':
                if (path[pos - 1] == '/') {
                    if (path[i + 1] == '.' && (path[i + 2] == '/' || path[i + 2] == '\\' || path[i + 2] == '\0')) {
                        while (--pos > 0) {
                            if (path[pos - 1] == '/') break;
                        }
                        if (pos < 1) return 0;
                        i += path[i + 2] == '\0' ? 1 : 2;
                        break;
                    }
                    if (path[i + 1] == '\0') break;
                    if (path[i + 1] == '/' || path[i + 1] == '\\') {
                        ++i;
                        break;
                    }
                }
                path[pos++] = '.';
#ifdef OS_WIN
                // windows does not have a trailing '.'
                sum = 1;
                while (path[i + sum] == '.') {
                    path[pos++] = '.';
                    ++sum;
                }
                if (path[i + sum] == '\0') pos -= sum;
                i += sum - 1;
#endif
                break;

            default:
#ifdef OS_WIN
                // windows is not case sensitive
                path[pos++] = (char)tolower(path[i]);
#else
                path[pos++] = path[i];
#endif
                break;
        }
    }
    path[pos] = '\0';
    return pos;
}
