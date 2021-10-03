# tradingo/replay

Dockerfile source: https://github.com/rorymcstay/tradingo/blob/master/docker/tradingo-replay.dockerfile

Build Details: https://github.com/rorymcstay/tradingo/actions/workflows/docker.yml

To see available run parameters. https://github.com/rorymcstay/tradingo/blob/master/app/scripts/profile.env#L22

STRATEGY environment variable determines which strategy configuration to run. It corresponds to

```
$INSTALL_LOCATION/etc/config/strategies/<name>.cfg
```

And in that file should be at minimum variable `factoryMethod`

```
factoryMethod=RegisterBreakOutStrategy
```

AWS credentials must also be provided through environment variables

- AWS_SECRET_ACCESS_TOKEN
- AWS_REGION
- AWS_ACCESS_KEY_ID

In order to access S3 bucket containing market data

The entry point should be

```
/usr/local/scripts/start_replay.sh 2021-10-02
```

# tradingo/base

Base image for all `tradingo` images. 

# trading

Actual `tradingo` runtime, runs similiarly to `tradingo/replay`, except that its base config connects to S3
and uses an slightly different compilation.
