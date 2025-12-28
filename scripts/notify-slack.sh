#!/bin/bash
# Send notification to Slack webhook
# Usage: ./scripts/notify-slack.sh "Message title" "Message text" [color]
#
# Set SLACK_WEBHOOK_URL environment variable or pass as 4th argument
# Example: export SLACK_WEBHOOK_URL="https://hooks.slack.com/services/..."
#          ./scripts/notify-slack.sh "Title" "Text" "good"
# Or:      ./scripts/notify-slack.sh "Title" "Text" "good" "https://hooks.slack.com/..."

WEBHOOK_URL="${4:-${SLACK_WEBHOOK_URL}}"

if [ -z "$WEBHOOK_URL" ]; then
  echo "Error: SLACK_WEBHOOK_URL environment variable not set and no webhook URL provided"
  echo "Usage: SLACK_WEBHOOK_URL=<url> $0 \"Title\" \"Text\" [color]"
  echo "   Or: $0 \"Title\" \"Text\" [color] <webhook_url>"
  exit 1
fi

TITLE="${1:-Notification}"
TEXT="${2:-Requires attention}"
COLOR="${3:-warning}"  # good, warning, danger

curl -X POST "$WEBHOOK_URL" \
  -H 'Content-Type: application/json' \
  -d "{
    \"attachments\": [{
      \"color\": \"$COLOR\",
      \"title\": \"⚠️ $TITLE\",
      \"text\": \"$TEXT\",
      \"fields\": [
        {
          \"title\": \"Time\",
          \"value\": \"$(date '+%Y-%m-%d %H:%M:%S')\",
          \"short\": true
        },
        {
          \"title\": \"Host\",
          \"value\": \"$(hostname)\",
          \"short\": true
        }
      ],
      \"footer\": \"TR4QT Development\"
    }]
  }"
