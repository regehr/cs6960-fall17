// PURPOSE: doubly linked list (intrusive container style)
// inspired by neat Linux kernel implementation

#define offsetof(x, member) __builtin_offsetof(x, member)


struct list {
        struct list *prev, *next;
};


// more robust then requiring to put struct list as first struct member
// TODO: Wikepedia typesafe cryptic solution
#define LIST_ENTRY(list, type, member) ((type *)((char *)list - offsetof(type, member)))


static inline void
list_init(struct list *const l)
{
        l->prev = l;
        l->next = l;
}


static inline int
list_empty(struct list const *const l)
{
        return l == l->next;
}


static inline struct list *
list_head(struct list *const l)
{
        return l->next;
}


// list_tail is bad name as we are modifying the list (unlike in Haskell)
static inline void
list_remove_head(struct list *const l)
{
       l->next->next->prev = l;
       l->next = l->next->next;
}


static inline void
list_append(struct list *const restrict l, struct list *const restrict elem)
{
        elem->prev = l->prev;
        elem->next = l;

        l->prev->next = elem;
        l->prev = elem;
}
