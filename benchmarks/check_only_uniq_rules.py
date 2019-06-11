
def check_only_uniq_rules(rule_lines):
    seen = set()
    for rl in rule_lines:
        rl = rl.strip()
        if rl in seen:
            print("duplicit:" + rl)
        seen.add(rl)

if __name__ == "__main__":
    fn = "../tests/data/acl1_100"
    with open(fn) as f:
        check_only_uniq_rules(f.readlines())