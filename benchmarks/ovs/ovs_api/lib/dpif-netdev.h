#pragma once


/* Stores a miniflow with inline values */

struct netdev_flow_key {
    uint32_t hash; /* Hash function differs for different users. */
    uint32_t len; /* Length of the following miniflow (incl. map). */
    struct miniflow mf;
    uint64_t buf[FLOW_MAX_PACKET_U64S];
};

struct dpcls {
    struct cmap_node node; /* Within dp_netdev_pmd_thread.classifiers */
    odp_port_t in_port;
    struct cmap subtables_map;
    struct pvector subtables;
};

/* A rule to be inserted to the classifier. */
struct dpcls_rule {
    struct cmap_node cmap_node; /* Within struct dpcls_subtable 'rules'. */
    struct netdev_flow_key *mask; /* Subtable's mask. */
    struct netdev_flow_key flow; /* Matching key. */
    /* 'flow' must be the last field, additional space is allocated here. */
};

void dpcls_init(struct dpcls*);
void dpcls_destroy(struct dpcls*);
void dpcls_sort_subtable_vector(struct dpcls*);
void dpcls_insert(struct dpcls*, struct dpcls_rule*,
        const struct netdev_flow_key *mask);
void dpcls_remove(struct dpcls*, struct dpcls_rule*);
bool dpcls_lookup(struct dpcls *cls, const struct netdev_flow_key *keys[],
        struct dpcls_rule **rules, size_t cnt, int *num_lookups_p);
