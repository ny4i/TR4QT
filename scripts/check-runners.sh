#!/bin/bash
# Self-Hosted Runner Status Check
# Validates that required self-hosted runners are online and ready
# Exit code 0 = all good, 1 = warning, 2 = critical (runner offline)

# Color codes for output
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo "=== TR4QT Self-Hosted Runner Status ==="
echo ""

# Check if gh CLI is available
if ! command -v gh >/dev/null 2>&1; then
    echo -e "${YELLOW}⚠ WARNING: GitHub CLI (gh) not installed${NC}"
    echo "  Cannot check runner status without gh CLI"
    echo "  Install: https://cli.github.com/"
    exit 1
fi

# Check if authenticated
if ! gh auth status >/dev/null 2>&1; then
    echo -e "${YELLOW}⚠ WARNING: Not authenticated with GitHub CLI${NC}"
    echo "  Run: gh auth login"
    exit 1
fi

# Get repository from git remote
REPO=$(gh repo view --json nameWithOwner -q '.nameWithOwner' 2>/dev/null || echo "")
if [ -z "$REPO" ]; then
    echo -e "${YELLOW}⚠ WARNING: Could not determine repository${NC}"
    echo "  Run this script from within the TR4QT repository"
    exit 1
fi

echo "Repository: $REPO"
echo ""

HAS_WARNINGS=0
HAS_ERRORS=0

# Function to check runner status from recent workflow runs
check_runner_health() {
    local RUNNER_NAME=$1
    local JOB_PATTERN=$2

    echo -e "${CYAN}Checking $RUNNER_NAME runner...${NC}"

    # Method 1: Try to query runners directly (requires admin)
    RUNNERS_JSON=$(gh api repos/$REPO/actions/runners 2>/dev/null || echo '{"runners":[]}')
    RUNNER_COUNT=$(echo "$RUNNERS_JSON" | grep -o '"id"' | wc -l | tr -d ' ')

    if [ "$RUNNER_COUNT" -gt 0 ]; then
        # Parse runner info
        echo "$RUNNERS_JSON" | python3 -c "
import sys, json
data = json.load(sys.stdin)
for r in data.get('runners', []):
    labels = [l['name'] for l in r.get('labels', [])]
    if 'self-hosted' in labels and 'linux' in [l.lower() for l in labels]:
        status = r.get('status', 'unknown')
        busy = r.get('busy', False)
        name = r.get('name', 'unknown')
        print(f'  Name: {name}')
        print(f'  Status: {status}')
        print(f'  Busy: {busy}')
        if status == 'online':
            if busy:
                print('  Result: BUSY')
            else:
                print('  Result: READY')
        else:
            print('  Result: OFFLINE')
" 2>/dev/null && return
    fi

    # Method 2: Infer from recent workflow runs
    echo "  (Checking via recent workflow runs...)"

    # Get the most recent run with Linux job
    LATEST_RUN=$(gh run list --limit 10 --json databaseId,status,conclusion,jobs 2>/dev/null)

    if [ -z "$LATEST_RUN" ] || [ "$LATEST_RUN" = "[]" ]; then
        echo -e "  ${YELLOW}⚠ Could not fetch recent runs${NC}"
        HAS_WARNINGS=1
        return
    fi

    # Check for Linux job status in recent runs
    LINUX_STATUS=$(echo "$LATEST_RUN" | python3 -c "
import sys, json
data = json.load(sys.stdin)
for run in data:
    for job in run.get('jobs', []):
        if 'Linux' in job.get('name', ''):
            status = job.get('status', '')
            conclusion = job.get('conclusion', '')
            if status == 'queued':
                print('QUEUED')
                sys.exit(0)
            elif status == 'in_progress':
                print('RUNNING')
                sys.exit(0)
            elif conclusion == 'success':
                print('SUCCESS')
                sys.exit(0)
            elif conclusion == 'failure':
                print('FAILED')
                sys.exit(0)
            elif conclusion == 'skipped':
                continue
            else:
                print(f'{status}:{conclusion}')
                sys.exit(0)
print('NO_RECENT_JOBS')
" 2>/dev/null)

    case "$LINUX_STATUS" in
        "SUCCESS")
            echo -e "  ${GREEN}✓ Recent Linux build succeeded - runner is functional${NC}"
            ;;
        "RUNNING")
            echo -e "  ${GREEN}✓ Linux build currently running - runner is online${NC}"
            ;;
        "QUEUED")
            echo -e "  ${YELLOW}⚠ Linux job queued - runner may be busy or starting up${NC}"
            HAS_WARNINGS=1
            ;;
        "FAILED")
            echo -e "  ${YELLOW}⚠ Recent Linux build failed - check build logs${NC}"
            HAS_WARNINGS=1
            ;;
        "NO_RECENT_JOBS")
            echo -e "  ${YELLOW}⚠ No recent Linux jobs found${NC}"
            HAS_WARNINGS=1
            ;;
        *)
            echo -e "  ${YELLOW}⚠ Unknown status: $LINUX_STATUS${NC}"
            HAS_WARNINGS=1
            ;;
    esac

    # Check if any Linux jobs are stuck in queue for too long
    QUEUED_RUNS=$(gh run list --status queued --limit 5 2>/dev/null)
    QUEUED_LINUX=$(echo "$QUEUED_RUNS" | grep -i "linux" | wc -l | tr -d ' ')

    if [ "$QUEUED_LINUX" -gt 0 ]; then
        # Check how long they've been queued
        echo -e "  ${YELLOW}⚠ $QUEUED_LINUX Linux job(s) in queue${NC}"

        # Get the oldest queued run time
        OLDEST_QUEUED=$(gh run list --status queued --limit 1 --json createdAt -q '.[0].createdAt' 2>/dev/null)
        if [ -n "$OLDEST_QUEUED" ]; then
            echo "  Oldest queued since: $OLDEST_QUEUED"
        fi

        HAS_WARNINGS=1
    fi
}

# Check Linux x86_64 runner
check_runner_health "Linux (x86_64)" "Linux"

echo ""

# Check workflow queue overall
echo -e "${CYAN}Checking workflow queue...${NC}"
QUEUED_COUNT=$(gh run list --status queued --json databaseId -q 'length' 2>/dev/null || echo "0")
IN_PROGRESS_COUNT=$(gh run list --status in_progress --json databaseId -q 'length' 2>/dev/null || echo "0")

echo "  Queued runs: $QUEUED_COUNT"
echo "  In-progress runs: $IN_PROGRESS_COUNT"

if [ "$QUEUED_COUNT" -gt 3 ]; then
    echo -e "  ${YELLOW}⚠ Multiple queued runs - runners may be overloaded or offline${NC}"
    HAS_WARNINGS=1
elif [ "$QUEUED_COUNT" -eq 0 ] && [ "$IN_PROGRESS_COUNT" -eq 0 ]; then
    echo -e "  ${GREEN}✓ No pending work - runners idle${NC}"
else
    echo -e "  ${GREEN}✓ Queue looks healthy${NC}"
fi

echo ""
echo "=== Summary ==="

if [ $HAS_ERRORS -gt 0 ]; then
    echo -e "${RED}✗ CRITICAL: One or more runners are offline or missing${NC}"
    echo ""
    echo "Troubleshooting steps:"
    echo "  1. SSH to the runner host and check the service:"
    echo "     sudo systemctl status actions.runner.*"
    echo "  2. Check runner logs:"
    echo "     journalctl -u actions.runner.* -n 50"
    echo "  3. Restart the runner service:"
    echo "     sudo systemctl restart actions.runner.*"
    echo "  4. For VM-based runner (linux-ci-build):"
    echo "     - Check VM is running in hypervisor UI"
    echo "     - Verify network connectivity"
    echo "  5. For Pi runner (bench5, if configured):"
    echo "     - Check chroot mounts: ls /opt/bookworm/proc"
    echo "     - Remount if needed: bash ~/mount-chroot.sh"
    exit 2
elif [ $HAS_WARNINGS -gt 0 ]; then
    echo -e "${YELLOW}⚠ Warnings detected - runners may have issues${NC}"
    echo "Builds might be delayed. Check runner status if issues persist."
    exit 1
else
    echo -e "${GREEN}✓ All runners appear healthy${NC}"
    exit 0
fi
