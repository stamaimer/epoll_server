```cpp
setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)
```

```cpp
struct timeval tv;
tv.tv_sec = 5;
tv.tv_usec = 0;
setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);
```