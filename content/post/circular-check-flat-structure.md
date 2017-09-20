+++
title = "环路检测"
date = "2017-09-08"
categories = ["algorithm"]
tags = ["algorithm", "dag"]
Authors = "Xudong"
draft = true
+++


项目中经常有一级二级三级等类似的tree结构，我们在入库的时候是以id, parentId的形势存的，在CURD的时候需要经常需要做成环检测来
规避环路。

实现的思路如下
1. 给每个节点/node/id做两个标记，一个visited标识是否访问过，一个stack标示当前检测栈，默认都设置为false。
2. 遍历所有的点(顺序无所谓), 做如下检测
        2.1. 首先标记该站点为visited，同时标记stack
        2.2. 找该节点的parent节点，如果没有访问过，跳到2.1递归调用(注意visited，和stack是全局共享的), 如果访问过并且stack以及
        标记了，说明检测到环了，程序到此结束了, 返回true
        2.3 如果没有检测到环，**标记stack为false**, 返回false
3. 如果都没有检测到，则没有环路

<!--more-->

注意这里stack和visited的应用，stack在第二步之前永远都是false的，相当于从tree的任意一个节点，一直递归到root，递归的过程中访问过
的点标记stack，如果某个stack以及是true了，然后又出现了一次，那就是检测到环路了，如果一直到root了还没有环路，需要将stack重新
置为false。

如下是一个简单的代码，输入的数据结构就是child/parent的键值对。

``` javascript
/**
 * obj structure: key is childId, value is related parentId
 *  {
 *    12: 1,
 *    123: 12,
 *    1234: 123,
 *    1235: 123,
 *    13: 1,
 *  },
 */
function isCircular(obj) {
  const visited = {}
  const stack = {}
  for (let key in obj) {
    visited[key] = false
    stack[key] = false
  }

  for (let key in obj) {
    if (!visited[key] && findCircular(key)) {
      return true
    }
  }
  return false

  function findCircular(key) {
    visited[key] = true
    stack[key] = true
    const parentKey = obj[key]
    if (parentKey) {
      if (!visited[parentKey]) {
        if (findCircular(parentKey))
          return true
      } else if (stack[parentKey]) {
        return true
      }
    }
    stack[key] = false
    return false
  }
}
```

其他的各种数据结构tree，dag的检测也可以套用这个方法。


### References
1. [geeksforgeeks detect cycle in a directed graph](http://www.geeksforgeeks.org/detect-cycle-in-a-graph/)
