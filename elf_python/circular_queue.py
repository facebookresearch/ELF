class CQueue:
    ''' Custom-made circular queue, which is fixed sized.
        The motivation here is to have
        * O(1) complexity for peeking the center part of the data
          In contrast, deque has O(n) complexity
        * O(1) complexity to sample from the queue (e.g., sampling replays)
    '''
    def __init__(self, n):
        ''' Preallocate n slots for circular queue '''
        self.n = n
        self.q = [None] * n
        # Data always pushed to the tail and popped from the head.
        # Head points to the next location to pop.
        # Tail points to the next location to push.
        # if head == tail, then #size = 0
        self.head = 0
        self.tail = 0
        self.sz = 0

    def _inc(self, v):
        v += 1
        if v >= self.n: v = 0
        return v

    def _dec(self, v):
        v -= 1
        if v < 0: v = self.n - 1
        return v

    def _proj(self, v):
        while v >= self.n:
            v -= self.n
        while v < 0:
            v += self.n

        return v

    def push(self, m):
        if self.sz == self.n: return False

        self.sz += 1

        self.q[self.tail] = m
        self.tail = self._inc(self.tail)

        return True

    def pop(self):
        if self.sz == 0: return

        self.sz -= 1

        m = self.q[self.head]
        self.head = self._inc(self.head)

        return m

    def popn(self, T):
        if self.sz < T: return
        self.sz -= T
        self.head = self._proj(self.head + T)
        return True

    def peekn_top(self, T):
        if self.sz < T: return
        return list(self.interval_pop(T))

    def peek_pop(self, start=0):
        if start >= self.sz: return
        i = self._proj(self.head + start)
        return self.q[i]

    def interval_pop(self, region_len, start=0):
        ''' Return an iterator with region_len
            The interval is self.head + [start, start+region_len)
        '''
        if self.sz < region_len - start:
            raise IndexError

        i = self._proj(self.head + start)
        for j in range(region_len):
            yield self.q[i]
            i = self._inc(i)

    def interval_pop_rev(self, region_len, end=-1):
        ''' Return a reverse iterator with region_len
            The interval is self.head + [end + region_len, end) (in reverse order)
        '''
        if self.sz < end + region_len:
            raise IndexError

        i = self._proj(self.head + end + region_len)
        for j in range(region_len):
            yield self.q[i]
            i = self._dec(i)

    def sample(self, T=1):
        ''' Sample from the queue for off-policy methods
            We want to sample T consecutive samples.
        '''
        start = random.randint(0, self.sz - T)

        # Return an iterator
        return self.interval_pop(T, start=start)

    def __len__(self):
        return self.sz

