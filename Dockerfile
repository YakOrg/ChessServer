FROM debian:slim
ADD chess-server /
ENTRYPOINT ["/chess-server"]