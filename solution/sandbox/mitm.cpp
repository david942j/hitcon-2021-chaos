// {{{ by david942j

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <utility>

#define mpr std::make_pair
#define lg(x) (31-__builtin_clz(x))
#define lgll(x) (63-__builtin_clzll(x))
#define __count __builtin_popcount
#define __countll __builtin_popcountll
#define X first
#define Y second
#define mst(x) memset(x,0,sizeof(x))
#define mst1(x) memset(x,-1,sizeof(x))
#define ALL(c) (c).begin(),(c).end()
#define FOR(i,n) for(int i=0;i<n;i++)
#define FOR1(i,n) for(int i=1;i<=n;i++)
#define FORit(it,c) for(auto it=(c).begin();it!=(c).end();++it)
#define pb push_back
#define RI(x) scanf("%d",&x)
#define RID(x) int x;RI(x)
using namespace std;

typedef long long LL;
typedef double LD;
typedef std::pair<int,int> PII;
template<class T>inline void maz(T &a,T b){if(a<b)a=b;}
template<class T>inline void miz(T &a,T b){if(a>b)a=b;}
template<class T>inline T abs(T a){return a>0?a:-a;}

#ifdef DAVID942J
template<typename T>
void _dump( const char* s, T&& head ) { cerr<<s<<"="<<head<<endl; }

template<typename T, typename... Args>
void _dump( const char* s, T&& head, Args&&... tail ) {
    int c=0;
    while ( *s!=',' || c!=0 ) {
        if ( *s=='(' || *s=='[' || *s=='{' ) c++;
        if ( *s==')' || *s==']' || *s=='}' ) c--;
        cerr<<*s++;
    }
    cerr<<"="<<head<<", ";
    _dump(s+1,tail...);
}

#define dump(...) do { \
    fprintf(stderr, "%s:%d - ", __PRETTY_FUNCTION__, __LINE__); \
    _dump(#__VA_ARGS__, __VA_ARGS__); \
} while (0)

template<typename Iter>
ostream& _out( ostream &s, Iter b, Iter e ) {
    s<<"[";
    for ( auto it=b; it!=e; it++ ) s<<(it==b?"":" ")<<*it;
    s<<"]";
    return s;
}

template<typename A, typename B>
ostream& operator <<( ostream &s, const pair<A,B> &p ) { return s<<"("<<p.first<<","<<p.second<<")"; }
template<typename T>
ostream& operator <<( ostream &s, const vector<T> &c ) { return _out(s,ALL(c)); }
template<typename T, size_t N>
ostream& operator <<( ostream &s, const array<T,N> &c ) { return _out(s,ALL(c)); }
template<typename T>
ostream& operator <<( ostream &s, const set<T> &c ) { return _out(s,ALL(c)); }
template<typename A, typename B>
ostream& operator <<( ostream &s, const map<A,B> &c ) { return _out(s,ALL(c)); }
#else
#define dump(...)
#endif

// }}} end of default code

namespace {

#define rotr(x,n) (((x) >> ((int)(n))) | ((x) << (64 - (int)(n))))
#define rotl(x,n) (((x) << ((int)(n))) | ((x) >> (64 - (int)(n))))

template <unsigned int C0, unsigned int C1>
inline void G256(uint64_t& G0, uint64_t& G1, uint64_t& G2, uint64_t& G3) {
  G0 += G1;
  G1 = rotl(G1, C0) ^ G0;
  G2 += G3;
  G3 = rotl(G3, C1) ^ G2;
}

template <unsigned int C0, unsigned int C1>
inline void IG256(uint64_t& G0, uint64_t& G1, uint64_t& G2, uint64_t& G3) {
  G3 = rotr(G3 ^ G2, C1);
  G2 -= G3;
  G1 = rotr(G1 ^ G0, C0);
  G0 -= G1;
}

#define KS256(r) \
    G0 += m_rkey[(r + 1) % 5]; \
    G1 += m_rkey[(r + 2) % 5]; \
    G2 += m_rkey[(r + 3) % 5]; \
    G3 += m_rkey[(r + 4) % 5] + r + 1;

#define IKS256(r) \
    G0 -= m_rkey[(r + 1) % 5]; \
    G1 -= m_rkey[(r + 2) % 5]; \
    G2 -= m_rkey[(r + 3) % 5]; \
    G3 -= m_rkey[(r + 4) % 5] + r + 1;

// #define GEN
// #define SORT

constexpr uint64_t m_rkey[5] = {
#ifdef GEN
    0,
#else
    1,
#endif
    0, 0, 0, 0x1BD11BDAA9FC1A22 ^ m_rkey[0] ^ m_rkey[1] ^ m_rkey[2] ^ m_rkey[3]};

void enc(uint64_t *data) {
  uint64_t G0 = data[0] + m_rkey[0];
  uint64_t G1 = data[1] + m_rkey[1];
  uint64_t G2 = data[2] + m_rkey[2];
  uint64_t G3 = data[3] + m_rkey[3];
  for (int i = 0; i < 9; i++) {
    G256<14, 16>(G0, G1, G2, G3);
    G256<52, 57>(G0, G3, G2, G1);
    G256<23, 40>(G0, G1, G2, G3);
    G256< 5, 37>(G0, G3, G2, G1);
    KS256(2 * i);
    G256<25, 33>(G0, G1, G2, G3);
    G256<46, 12>(G0, G3, G2, G1);
    G256<58, 22>(G0, G1, G2, G3);
    G256<32, 32>(G0, G3, G2, G1);
    KS256(2 * i + 1);
  }
  data[0] = G0;
  data[1] = G1;
  data[2] = G2;
  data[3] = G3;
}

void dec(uint64_t *data) {
  uint64_t G0 = data[0] - m_rkey[3];
  uint64_t G1 = data[1] - m_rkey[4];
  uint64_t G2 = data[2] - m_rkey[0];
  uint64_t G3 = data[3] - m_rkey[1] - 18;
  for (int i = 8; i >= 0; i--) {
    IG256<32, 32>(G0, G3, G2, G1);
    IG256<58, 22>(G0, G1, G2, G3);
    IG256<46, 12>(G0, G3, G2, G1);
    IG256<25, 33>(G0, G1, G2, G3);
    IKS256(2 * i);
    IG256< 5, 37>(G0, G3, G2, G1);
    IG256<23, 40>(G0, G1, G2, G3);
    IG256<52, 57>(G0, G3, G2, G1);
    IG256<14, 16>(G0, G1, G2, G3);
    IKS256(2 * i - 1);
  }
  data[0] = G0;
  data[1] = G1;
  data[2] = G2;
  data[3] = G3;
}

void gen() {
#define BATCH (1u << 22)
  static uint64_t buffer[BATCH];
  uint32_t b = 0;
  int fd = open("gen.data", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  assert(fd >= 0);
  for (uint64_t i = 0; i < 1ul << 32; i++) {
    uint64_t data[4] = {0, 0, 0, 0x21};
    data[0] = i;
    enc(data);
    buffer[b++] = data[3];
    if (b == BATCH) {
      fprintf(stderr, "0x%lx\n", i);
      write(fd, buffer, sizeof(buffer));
      b = 0;
    }
  }
}

void sort() {
  // cp gen.data sort.data before run this func
  int fd = open("sort.data", O_RDWR);
  uint64_t *buf = (uint64_t *)mmap(0, 8ull << 32, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  std::sort(buf, buf + (1ull << 32));
}

template <typename T>
int compare(const void *a, const void *b) {
    const auto &arg1 = *(static_cast<const T*>(a));
    const auto &arg2 = *(static_cast<const T*>(b));
    if (arg1 < arg2) return -1;
    else if (arg1 > arg2) return 1;
    else return 0;
}

void match(uint64_t v) {
  int fd = open("sort.data", O_RDONLY);
  uint64_t *buf = (uint64_t *)mmap(0, 8ull << 32, PROT_READ, MAP_SHARED, fd, 0);

  fprintf(stderr, "Running with v=0x%lx\n", v);
  for (uint64_t i = 0; i < 1ul << 32; i++) {
    uint64_t data[4] = {i, v, 0, 0x81};
    dec(data);
    const uint64_t *p = static_cast<uint64_t *>(
      std::bsearch(&data[3], buf, 1ull << 32, sizeof(uint64_t), compare<uint64_t>));
    if (p) {
      printf("Found a match! 0x%lx -> 0x%lx\n", i, data[3]);
      return;
    }
    if (i % (1ul << 22) == 0)
      fprintf(stderr, "0x%lx\n", i);
  }
}

} // namespace

int main(int argc, char *argv[]) {
  uint64_t data[4] = {0x2119892, 3, 0, 0x81};
  dec(data);
  for(int i = 0; i < 4; i++)
      printf("0x%lx\n", data[i]);
//   int fd = open("gen.data", O_RDONLY);
//   uint64_t *buf = (uint64_t *)mmap(0, 8ull << 32, PROT_READ, MAP_SHARED, fd, 0);
//   uint64_t tar = 0x15f54ba25899926cull;
//   for (uint64_t i = 0; i < 1ull << 32; i++) {
//     if (buf[i] == tar) {
//       printf("Got 0x%lx at index 0x%lx\n", tar, i);
//       return 0;
//     }
//   }
// #ifdef GEN
//   gen();
// #elif defined(SORT)
//   sort();
// #else
//   if (argc != 2) { fprintf(stderr, "Usage: %s <num>\n", argv[0]); }
//   match(atoi(argv[1]));
// #endif
  return 0;
}

