source "$(dirname ${BASH_SOURCE[0]})/profile.env"

replay_tradingo_on() {

    aws s3 cp "s3://$BUCKET_NAME/tickRecorder/storage/2021-09-08/quotes_$SYMBOL.json" "$TICK_STORAGE"
    REPLAY_STORAGE=$REPLAY_STORAGE DATESTR=$1 envsubst < /usr/etc/config/replayTradingo.cfg  > /tmp/replay.cfg
    cat /tmp/replay.cfg
    replayTradingo --config /tmp/replay.cfg &> "/tmp/log/replay_$1_$(date -I).log"
    aws s3 sync "$REPLAY_STORAGE" "s3://$BUCKET_NAME/replays/"
}
replay_tradingo_on $1