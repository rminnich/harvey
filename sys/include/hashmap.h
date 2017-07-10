
typedef struct Hashentry Hashentry;
typedef struct Hashtable Hashtable;
typedef struct Hashmap Hashmap;

struct Hashentry {
	uint64_t key;
	uint64_t val;
};

struct Hashtable {
	Hashentry *tab;
	size_t len;
	size_t cap; // always a power of 2.
};

struct Hashmap {
	Hashtable *cur;
	Hashtable *next;
	Hashtable tabs[2];
};

typedef char* (*applyfunc)(Hashentry *, void *);
char * hmapinit(Hashmap *ht);
char * hmapfree(Hashmap *ht);
char * hmapdel(Hashmap *ht, uint64_t key, uint64_t *valp);
char * hmapget(Hashmap *ht, uint64_t key, uint64_t *valp);
char * hmapput(Hashmap *ht, uint64_t key, uint64_t val);
char * hmapapply(Hashmap *ht, applyfunc f, void *arg);
char * hmapstats(Hashmap *ht, size_t *lens, size_t nlens);
