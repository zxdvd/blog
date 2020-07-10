
BASE32 = "0123456789bcdefghjkmnpqrstuvwxyz"        # 0-9a-z except ailo (36-4=32)

MIN_LNG = -180
MAX_LNG = 180
MIN_LAT = -90
MAX_LAT = 90

def geo_hash(lng, lat, rounds):
    result = ''
    bits = 0
    i = 0
    min_lng = MIN_LNG
    max_lng = MAX_LNG
    min_lat = MIN_LAT
    max_lat = MAX_LAT

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

        # generate two base32 char every 5 rounds
        if i % 5 == 0:
            result += BASE32[bits >> 5]
            result += BASE32[bits & 31]
            bits = 0                # reset bits

    m = i % 5
    if m <= 2:                  # less or equals to 4 bits, only one base32 char
        result += BASE32[bits & 31]
    if m == 3:                  # 6 bits left, two base32 char
        result += BASE32[bits >> 1]      # top 5 bits
        result += BASE32[bits & 1]       # lower 1 bit
    if m == 4:                  # 8 bits left, two base32 char
        result += BASE32[bits >> 3]      # top 5 bits
        result += BASE32[bits & 0b111]       # lower 3 bits

    return result

print(geo_hash(13.361389, 38.115556, 24))
print(geo_hash(-5.6, 42.6, 28))
