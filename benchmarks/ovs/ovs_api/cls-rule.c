#include "classifier.h"


/* cls_rule. */

static inline void
cls_rule_init__(struct cls_rule *rule, unsigned int priority)
{
    rculist_init(&rule->node);
    *CONST_CAST(int *, &rule->priority) = priority;
    ovsrcu_init(&rule->cls_match, NULL);
}

/* Initializes 'rule' to match packets specified by 'match' at the given
 * 'priority'.  'match' must satisfy the invariant described in the comment at
 * the definition of struct match.
 *
 * The caller must eventually destroy 'rule' with cls_rule_destroy().
 *
 * Clients should not use priority INT_MIN.  (OpenFlow uses priorities between
 * 0 and UINT16_MAX, inclusive.) */
void
cls_rule_init(struct cls_rule *rule, const struct match *match, int priority)
{
    cls_rule_init__(rule, priority);
    minimatch_init(CONST_CAST(struct minimatch *, &rule->match), match);
}

/* Same as cls_rule_init() for initialization from a "struct minimatch". */
void
cls_rule_init_from_minimatch(struct cls_rule *rule,
                             const struct minimatch *match, int priority)
{
    cls_rule_init__(rule, priority);
    minimatch_clone(CONST_CAST(struct minimatch *, &rule->match), match);
}

/* Initializes 'dst' as a copy of 'src'.
 *
 * The caller must eventually destroy 'dst' with cls_rule_destroy(). */
void
cls_rule_clone(struct cls_rule *dst, const struct cls_rule *src)
{
    cls_rule_init__(dst, src->priority);
    minimatch_clone(CONST_CAST(struct minimatch *, &dst->match), &src->match);
}

/* Initializes 'dst' with the data in 'src', destroying 'src'.
 *
 * 'src' must be a cls_rule NOT in a classifier.
 *
 * The caller must eventually destroy 'dst' with cls_rule_destroy(). */
void
cls_rule_move(struct cls_rule *dst, struct cls_rule *src)
{
    cls_rule_init__(dst, src->priority);
    minimatch_move(CONST_CAST(struct minimatch *, &dst->match),
                   CONST_CAST(struct minimatch *, &src->match));
}

/* Frees memory referenced by 'rule'.  Doesn't free 'rule' itself (it's
 * normally embedded into a larger structure).
 *
 * ('rule' must not currently be in a classifier.) */
void
cls_rule_destroy(struct cls_rule *rule)
    OVS_NO_THREAD_SAFETY_ANALYSIS
{
    /* Must not be in a classifier. */
    ovs_assert(!get_cls_match_protected(rule));

    /* Check that the rule has been properly removed from the classifier. */
    ovs_assert(rule->node.prev == RCULIST_POISON
               || rculist_is_empty(&rule->node));
    rculist_poison__(&rule->node);   /* Poisons also the next pointer. */

    minimatch_destroy(CONST_CAST(struct minimatch *, &rule->match));
}

/* This may only be called by the exclusive writer. */
void
cls_rule_set_conjunctions(struct cls_rule *cr,
                          const struct cls_conjunction *conj, size_t n)
{
    struct cls_match *match = get_cls_match_protected(cr);
    struct cls_conjunction_set *old
        = ovsrcu_get_protected(struct cls_conjunction_set *, &match->conj_set);
    struct cls_conjunction *old_conj = old ? old->conj : NULL;
    unsigned int old_n = old ? old->n : 0;

    if (old_n != n || (n && memcmp(old_conj, conj, n * sizeof *conj))) {
        if (old) {
            ovsrcu_postpone(free, old);
        }
        ovsrcu_set(&match->conj_set,
                   cls_conjunction_set_alloc(match, conj, n));
    }
}


/* Returns true if 'a' and 'b' match the same packets at the same priority,
 * false if they differ in some way. */
bool
cls_rule_equal(const struct cls_rule *a, const struct cls_rule *b)
{
    return a->priority == b->priority && minimatch_equal(&a->match, &b->match);
}

/* Appends a string describing 'rule' to 's'. */
void
cls_rule_format(const struct cls_rule *rule, const struct tun_table *tun_table,
                const struct ofputil_port_map *port_map, struct ds *s)
{
    minimatch_format(&rule->match, tun_table, port_map, s, rule->priority);
}

/* Returns true if 'rule' matches every packet, false otherwise. */
bool
cls_rule_is_catchall(const struct cls_rule *rule)
{
    return minimask_is_catchall(rule->match.mask);
}

/* Makes 'rule' invisible in 'remove_version'.  Once that version is used in
 * lookups, the caller should remove 'rule' via ovsrcu_postpone().
 *
 * 'rule' must be in a classifier.
 * This may only be called by the exclusive writer. */
void
cls_rule_make_invisible_in_version(const struct cls_rule *rule,
                                   ovs_version_t remove_version)
{
    struct cls_match *cls_match = get_cls_match_protected(rule);

    ovs_assert(remove_version >= cls_match->versions.add_version);

    cls_match_set_remove_version(cls_match, remove_version);
}

/* This undoes the change made by cls_rule_make_invisible_in_version().
 *
 * 'rule' must be in a classifier.
 * This may only be called by the exclusive writer. */
void
cls_rule_restore_visibility(const struct cls_rule *rule)
{
    cls_match_set_remove_version(get_cls_match_protected(rule),
                                 OVS_VERSION_NOT_REMOVED);
}

/* Return true if 'rule' is visible in 'version'.
 *
 * 'rule' must be in a classifier. */
bool
cls_rule_visible_in_version(const struct cls_rule *rule, ovs_version_t version)
{
    struct cls_match *cls_match = get_cls_match(rule);

    return cls_match && cls_match_visible_in_version(cls_match, version);
}

/* Returns true if 'rule' exactly matches 'criteria' or if 'rule' is more
 * specific than 'criteria'.  That is, 'rule' matches 'criteria' and this
 * function returns true if, for every field:
 *
 *   - 'criteria' and 'rule' specify the same (non-wildcarded) value for the
 *     field, or
 *
 *   - 'criteria' wildcards the field,
 *
 * Conversely, 'rule' does not match 'criteria' and this function returns false
 * if, for at least one field:
 *
 *   - 'criteria' and 'rule' specify different values for the field, or
 *
 *   - 'criteria' specifies a value for the field but 'rule' wildcards it.
 *
 * Equivalently, the truth table for whether a field matches is:
 *
 *                                     rule
 *
 *                   c         wildcard    exact
 *                   r        +---------+---------+
 *                   i   wild |   yes   |   yes   |
 *                   t   card |         |         |
 *                   e        +---------+---------+
 *                   r  exact |    no   |if values|
 *                   i        |         |are equal|
 *                   a        +---------+---------+
 *
 * This is the matching rule used by OpenFlow 1.0 non-strict OFPT_FLOW_MOD
 * commands and by OpenFlow 1.0 aggregate and flow stats.
 *
 * Ignores rule->priority. */
bool
cls_rule_is_loose_match(const struct cls_rule *rule,
                        const struct minimatch *criteria)
{
    return (!minimask_has_extra(rule->match.mask, criteria->mask)
            && miniflow_equal_in_minimask(rule->match.flow, criteria->flow,
                                          criteria->mask));
}
