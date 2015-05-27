# srs-dolphin

The dolphin is a MultipleProcess for [SRS][SRS].

## About

The dolphin is [the most simple MultipleProcess arch][ARCH] for RTMP/HTTP-FLV of SRS.

The [srs-sharp][SHARP] is a similar project, but srs-sharp is a HTTP FLV reverse proxy, its performance is great and is's ok for HTTP FLV stream. The bellow table is a comparation between srs-sharp and srs-dolphin:

|   Project   |   Goal        |   Protocol    |   Performance   |   Deploy    |
|   -------   |   -----       |   --------    |   -----------   |   -------   |
|   srs-sharp | HTTP-FLV      |   HTTP        |   10k, 300%CPU  |   Manual(*) |
| srs-dolphin | RTMP/HTTP-FLV |   TCP         |   -             |   Auto(*)   |

Remark:

1. Manual Deploy of srs-sharp: User should manually deply the SRS edge, then start srs-sharp to proxy the ports of SRS.
1. Auto Deploy of srs-dolphin: User only need to start srs-dophin, which will auto manage the SRS.

Winlin 2015.5

[SRS]: https://github.com/simple-rtmp-server/srs
[ARCH]: https://github.com/simple-rtmp-server/srs/wiki/v3_CN_Architecture#multiple-processes-planb
[SHARP]: https://github.com/simple-rtmp-server/go-sharp
