FROM debian:stable-slim
ADD chess-server /
ENTRYPOINT ["/chess-server"]