```metadata
tags: data-structure, geohash, z-order curve
```

## geohash
Geohash is a encoding mechanism that encodes multi-dimension geographic location
 (longtitude, latitude) to one dimension short string.

The geohash has a nice property that the longer the shared prefix of two location is,
 the closer they are. You can use this feature to query nearby neighbors. However,
 it does guarantee that nearby neighbors will have common prefix. I'll explain this
 later.

### geohash algorithm
The underground algorithm is something similar to z-order curve, morton code. The basic
 idea is explained as following:

- suppose you have multi-dimension data like `(x1x2x3...x31x32, y1y2...y31y32, z1z2...z31z32)`,
 the x1, y1, z1 are bit here (0 or 1).

- generate a new bit array as `x1y1z1  x2y2z2  ...  x31y31z31  x32y32z32`.

- convert to string (optional, geohash converts it to string using base32 encoding)

For the generated bit array, we know that the longer the shared prefix of two bit arrays
 is, the near the origin points (X,Y,Z) are.

I wrote a demo geohash script at [here](./scripts/geohash.py). The core part is:

```python
    while i <= rounds:
        i += 1

        mid_lng = (min_lng + max_lng) / 2
        if lng >= mid_lng:
            bits = (bits << 1) + 1    # append a 1
            min_lng = mid_lng
        else:
            bits =  bits << 1         # append a 0
            max_lng = mid_lng

        mid_lat = (min_lat + max_lat) / 2
        if lat >= mid_lat:
            bits = (bits << 1) + 1    # append a 1
            min_lat = mid_lat
        else:
            bits =  bits << 1         # append a 0
            max_lat = mid_lat
```

Divided longtitude and latitude space to two parts, then we got a bit 1 if longtitude
 of the point is in the bigger part otherwise 0. Got another bit from latitude.
 And then repeat this at the chosen space until got enough bits.

### edge cases
For two geohash `bcdefgh` and `bcdefgj` we can claim that they are close to each other
 since they shared a long prefix `bcdefg`. Then can we claim that any two close locations
 will have a proper shared prefix?

The answer is NO. Think about two close location like `P1=(lng=-0.1, lat=0)` and
 `P2=(lng=0.1, lat=0)`. We can ignore the latitude since they are same. Let's calculate
 the first bit. The middle longtitude is 0 ((-180+180)/2), so first bit of P1 is 0 since
 `-0.1 < 0` but first bit of P2 is 1 since `0.1 > 0`. Since the first bit of the bit array
 is different, then they don't have shared prefix. From geohash's point of view, they are
 not close to each other, totally. But actually they are.

From above, we can find that this problem happens at two sides of the dividing boundary.
For geohash, this problem is extremely serious at two poles.

### precision
From above explaination, we know that geohash can only give you probable result. It can
 not give you exactly matched result. So if you want exactly result like the top 10 nestest
 neighbors, you may need other data structure like [rtree](./rtree introduction.md).

### geohash vs rtree
Geohash is simple and very fast. But the bad side is that the result is not so exactly.
Rtree is a little complicated but it gives you exactly result.

Rtree can do more complexed operations like range query, line and polygon query.

### references
[wikipedia: geohash](https://en.wikipedia.org/wiki/Geohash)
[blog: how geohash work](https://www.factual.com/blog/how-geohashes-work/)
