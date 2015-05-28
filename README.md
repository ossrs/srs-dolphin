# srs-dolphin

The dolphin is a MultipleProcess for [SRS][SRS].

## About

The dolphin is [the most simple MultipleProcess arch][ARCH] for RTMP/HTTP-FLV of SRS.

The [srs-sharp][SHARP] is a similar project, but srs-sharp is a HTTP FLV reverse proxy, its performance is great and is's ok for HTTP FLV stream. The bellow table is a comparation between srs-sharp and srs-dolphin:

|   Project   |   Goal        |   Protocol    |   Performance   |   Deploy    |
|   -------   |   -----       |   --------    |   -----------   |   -------   |
|   srs-sharp | HTTP-FLV      |   HTTP        |   10k, 300%CPU  |   Manual(*) |
| srs-dolphin | RTMP/HTTP-FLV |   TCP         |   20k, 320%CPU  |   Auto(*)   |

Remark:

1. **Manual** Deploy of srs-sharp: User should manually deply the SRS edge, then start srs-sharp to proxy the ports of SRS.
1. **Auto** Deploy of srs-dolphin: User only need to start srs-dophin, which will auto manage the SRS.

## Usage

**Step 1:** Prepare SRS:

```
cd ~ && git clone https://github.com/simple-rtmp-server/srs.git &&
cd ~/srs/trunk && ./configure --with-ffmpeg && make && ./objs/srs -c conf/srs.conf
```

Remark: We clone the SRS and Dolphin to user home, user can specify others.

**Step 2:** Clone the srs-dolphin:

```
cd ~ && git clone https://github.com/simple-rtmp-server/srs-dolphin.git
```

**Step 3:** Build and run the srs-dolphin:

```
cd ~/srs-dolphin/trunk && make && 
./objs/srs_dolphin -w 4 -p 19350 -x 8088 \
    -b ~/srs/trunk/objs/srs -c conf/dolphin.conf \
    -s 1936,1937,1938,1939 -y 8081,8082,8083,8084
```

Remark: User can specifies the SRS in other valid path.<br/>
Remark: The conf/dolphin.conf is a default conf for SRS by dolphin, user can use others.

**Step 4:** Publish stream

```
cd ~/srs/trunk &&
for ((;;)); do
    ./objs/ffmpeg/bin/ffmpeg -re -i ./doc/source.200kbps.768x320.flv \
        -vcodec copy -acodec copy -f flv -y rtmp://127.0.0.1:1935/live/livestream
done
```

**Step 5:** Play stream

```
Origin SRS RTMP stream: rtmp://127.0.0.1:1935/live/livestream
Edge Dolphin RTMP stream: rtmp://127.0.0.1:19350/live/livestream
Edge Dolphin HTTP-FLV stream: http://127.0.0.1:8088/live/livestream.flv
```

Remark: User can use [SB][SB] to do the benchmark.

## Features

1. Multiple Processes for SRS edge.
1. Delivery stream in RTMP.
1. Delivery stream in HTTP FLV.
1. [dev] Support HTTP API for Multiple Processes.
1. [dev] Muktiple Processes for SRS origin.
1. [dev] Auto fork new process when worker or SRS exit.
1. [dev] Load balance proxy for RTMP and HTTP-FLV.

Winlin 2015.5

[SRS]: https://github.com/simple-rtmp-server/srs
[ARCH]: https://github.com/simple-rtmp-server/srs/wiki/v3_CN_Architecture#multiple-processes-planb
[SHARP]: https://github.com/simple-rtmp-server/go-sharp
[SB]: https://github.com/simple-rtmp-server/srs-bench
