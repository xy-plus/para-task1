# 运行方式

```sh
bash run.sh config/bcast.txt
```

# 运行结果

bcast 5GB/s

gather 2GB/s

gather 比 bacast 慢，应该是因为接收信息的结点压力太大有堵塞。

reduce_sum 2.7GB/s
