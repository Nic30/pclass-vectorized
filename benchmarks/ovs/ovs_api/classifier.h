#ifndef CLASSIFIER_PCV_H
#define CLASSIFIER_PCV_H 1

#include "openvswitch/match.h"
#include "openvswitch/meta-flow.h"
#include "pvector.h"
#include "rculist.h"
#include "openvswitch/type-props.h"
#include "versions.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Classifier internal data structures. */
struct cls_match;

/* A flow classifier. */
struct classifier {
	void * priv; /* [TODO] place object data directly in struct, problem: resolve size of cls in C */
    bool publish;                   /* Make changes visible to lookups? */
};

struct cls_conjunction {
    uint32_t id;
    uint8_t clause;
    uint8_t n_clauses;
};

/* A rule to be inserted to the classifier. */
struct cls_rule {
    const int priority;           /* Larger numbers are higher priorities. */
    const struct minimatch match; /* Matching rule. */
};

/* Constructor/destructor.  Must run single-threaded. */
void classifier_init(struct classifier *, const uint8_t *flow_segments);
void classifier_destroy(struct classifier *);

/* Modifiers.  Caller MUST exclude concurrent calls from other threads. */
bool classifier_set_prefix_fields(struct classifier *,
                                  const enum mf_field_id *trie_fields,
                                  unsigned int n_trie_fields);

void cls_rule_init(struct cls_rule *, const struct match *, int priority);
void cls_rule_init_from_minimatch(struct cls_rule *, const struct minimatch *,
                                  int priority);
void cls_rule_clone(struct cls_rule *, const struct cls_rule *);
void cls_rule_move(struct cls_rule *dst, struct cls_rule *src);
void cls_rule_destroy(struct cls_rule *);

void cls_rule_set_conjunctions(struct cls_rule *,
                               const struct cls_conjunction *, size_t n);
void cls_rule_make_invisible_in_version(const struct cls_rule *,
                                        ovs_version_t);
void cls_rule_restore_visibility(const struct cls_rule *);

void classifier_insert(struct classifier *, const struct cls_rule *,
                       ovs_version_t, const struct cls_conjunction *,
                       size_t n_conjunctions);
const struct cls_rule *classifier_replace(struct classifier *,
                                          const struct cls_rule *,
                                          ovs_version_t,
                                          const struct cls_conjunction *,
                                          size_t n_conjunctions);
bool classifier_remove(struct classifier *, const struct cls_rule *);
void classifier_remove_assert(struct classifier *, const struct cls_rule *);
static inline void classifier_defer(struct classifier *);
static inline void classifier_publish(struct classifier *);

/* Lookups.  These are RCU protected and may run concurrently with modifiers
 * and each other. */
const struct cls_rule *classifier_lookup(const struct classifier *,
                                         ovs_version_t, struct flow *,
                                         struct flow_wildcards *);
bool classifier_rule_overlaps(const struct classifier *,
                              const struct cls_rule *, ovs_version_t);
const struct cls_rule *classifier_find_rule_exactly(const struct classifier *,
                                                    const struct cls_rule *,
                                                    ovs_version_t);
const struct cls_rule *classifier_find_match_exactly(const struct classifier *,
                                                     const struct match *,
                                                     int priority,
                                                     ovs_version_t);
const struct cls_rule *classifier_find_minimatch_exactly(
    const struct classifier *, const struct minimatch *,
    int priority, ovs_version_t);

bool classifier_is_empty(const struct classifier *);
int classifier_count(const struct classifier *);

/* Classifier rule properties.  These are RCU protected and may run
 * concurrently with modifiers and each other. */
bool cls_rule_equal(const struct cls_rule *, const struct cls_rule *);
void cls_rule_format(const struct cls_rule *, const struct tun_table *,
                     const struct ofputil_port_map *, struct ds *);
bool cls_rule_is_catchall(const struct cls_rule *);
bool cls_rule_is_loose_match(const struct cls_rule *rule,
                             const struct minimatch *criteria);
bool cls_rule_visible_in_version(const struct cls_rule *, ovs_version_t);

/* Iteration.
 *
 * Iteration is lockless and RCU-protected.  Concurrent threads may perform all
 * kinds of concurrent modifications without ruining the iteration.  Obviously,
 * any modifications may or may not be visible to the concurrent iterator, but
 * all the rules not deleted are visited by the iteration.  The iterating
 * thread may also modify the classifier rules itself.
 *
 * 'TARGET' iteration only iterates rules matching the 'TARGET' criteria.
 * Rather than looping through all the rules and skipping ones that can't
 * match, 'TARGET' iteration skips whole subtables, if the 'TARGET' happens to
 * be more specific than the subtable. */
struct cls_cursor {
    const struct classifier *cls;
    const struct cls_rule *target;
    void * pos;
    const struct cls_rule *rule;
};

struct cls_cursor cls_cursor_start(const struct classifier *,
                                   const struct cls_rule *target,
                                   ovs_version_t);
void cls_cursor_advance(struct cls_cursor *);

#define CLS_FOR_EACH(RULE, MEMBER, CLS)             \
    CLS_FOR_EACH_TARGET(RULE, MEMBER, CLS, NULL, OVS_VERSION_MAX)
#define CLS_FOR_EACH_TARGET(RULE, MEMBER, CLS, TARGET, VERSION)         \
    for (struct cls_cursor cursor__ = cls_cursor_start(CLS, TARGET, VERSION); \
         (cursor__.rule                                                 \
          ? (INIT_CONTAINER(RULE, cursor__.rule, MEMBER),               \
             cls_cursor_advance(&cursor__),                             \
             true)                                                      \
          : false);                                                     \
        )


static inline void
classifier_defer(struct classifier *cls)
{
    cls->publish = false;
}

static inline void
classifier_publish(struct classifier *cls)
{
    cls->publish = true;
}

#ifdef __cplusplus
}
#endif
#endif /* classifier.h */
