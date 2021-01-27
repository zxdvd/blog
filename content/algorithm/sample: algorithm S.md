```metadata
tags: algorithm, sample, knuth
```

## sample: algorithm S

I learnt about algorithm S when I want to know how postgres's analysis works and I
 found that it uses the algorithm S (from Knuth 3.4.2) to get sample rows of whole
 table and analyze.

The `algorithm S` takes M sample of N elements while N is not known until finishing.
The key point is `N is not known`. It works very well when the input is stream data.

Suppose you want to get m samples from n elements (n is not known).

- Get first m from all elements, if n <= m, then all of them are used.

- For the i-th element (i>m), it has probability of `m/i` to be chosen to fill in the
 samples, and at the same time one of the m samples will be evicted randomly.

- Repeat above step for all data.

Looks very simple, but is it uniform sampling? Let's prove it.

For the i-th element, it has probability of `m/i` to be temporarily chosen, but what's
 the probability that it will be kept finally?

- At the i+1 step, `m/i+1` is the probability that i+1 will be chosen, `1-m/(i+1)` is
 the probability that it will not be chosen.

    + if i+1 element is not chosen (1-m/(i+1)), then of course the i-th element is kept
      since no need to evict one from previously chosen elements
    + if i+1 element is chosen, then probability that i-th will be kept is `m/(i+1) * (1-1/m)`
    + since the probability of i-th will not be evicted is `1-1/m`
    + sum up above two, the probability that i-th will survive in i+1 step is

            1-m/(i+1)  +  m/(i+1) * (1-1/m)
            = (i+1-m)/(i+1)  + m/(i+1) * ((m-1)/m)
            = (i+1-m)/(i+1)  + (m-1)/(i+1)
            = i/(i+1)

- Then in i+2 step, the probability is `(i+1)/(i+2)` and in final step, it is `n-1/n`.

- So for the i-th element, the probability that it will be kept finally is

        m/i * (i/i+1) * (i+1/i+2) * (i+2/i+3) * ... (n-1)/n = m/n

- For the first m elements, the probability is

        1 * (m/m+1) * (m+1/m+2) * ... (n-1)/n = m/n

- So we can conclude that for any element, the sample probability is evenly `m/n`.




