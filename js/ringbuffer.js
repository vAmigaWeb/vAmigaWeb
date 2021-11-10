//ported from Emulator/Utilities/RingBuffer.h and speed optimized for Javascript mithrendal
class RingBuffer {
    constructor(capacity) {
        this.capacity = capacity+1;
        this.capacity_usable = capacity;
        this.r = 0;
        this.w = 0;
        this.elements = new ArrayBuffer(this.capacity);
    }
 
    clear(){
        this.r = 0;
        this.w = 0;
        for (let i = 0; i < this.capacity; i++) { this.elements[i] = null; }
    }
 
    align(offset) {
        this.w = (this.r + offset) % this.capacity;
    }
 
    count() {
        return (this.capacity + this.w - this.r) % this.capacity;
    }
    free()  { return this.capacity - this.count() - 1; }
    fillLevel() { return this.count() / this.capacity; }
    isEmpty()  { return this.r == this.w; }
    isFull()  { return this.count() == this.capacity - 1; }
 
    begin()  { return this.r; }
    end()  { return this.w; }
    next(i) { return i < this.capacity - 1 ? i + 1 : 0; }
    prev(i) { return i > 0 ? i - 1 : this.capacity - 1; }
 
    read()
    {
//        console.assert(!this.isEmpty(), "Ringbuffer.read() -> !isEmpty()");
 
        let oldr = this.r;
        //this.r = this.next(this.r);
        this.r = this.r < this.capacity_usable ? this.r + 1 : 0;
        return this.elements[oldr];
    }
 
    write(element)
    {
//        console.log("write "+element)
//        console.assert(!this.isFull(), "Ringbuffer.write() -> !isFull()");
 
        this.elements[this.w] = element;
        //this.w = this.next(this.w);
        this.w = this.w < this.capacity_usable ? this.w + 1 : 0;
    }
  
    skip()
    {
        this.r = this.next(this.r);
    }
   
    skip(n)
    {
        this.r = (this.r + n) % this.capacity;
    }
   
    current(offset)
    {
        return this.elements[(this.r + offset) % this.capacity];
    }
}

