class vAmigaAudioProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.port.onmessage = this.handleMessage.bind(this);
    this.port.onmessageerror = (error)=>{ 
      console.error("error:"+ error);
    };

    this.fetch_buffer=null;
    this.buffer=null;
    this.recyle_buffer;;
    this.buf_addr=0;
    this.pending_post=false;
    console.log("vAmiga_audioprocessor connected");
  }

  handleMessage(event) {
  //  console.log("processor received sound data");
    this.fetch_buffer = event.data;
    this.pending_post=false;
  }

  fetch_data()
  {
//    console.log("audio processor: fetch");
    this.pending_post=true;
    if(this.recyle_buffer==null)
    {
      console.log("audio processor has no recycled buffer... creates new buffer");
      this.recyle_buffer=new Float32Array(4096*2);
    }
    this.port.postMessage(this.recyle_buffer, [this.recyle_buffer.buffer]);
    this.recyle_buffer=null;
  }


  process(inputs, outputs) {
    if(this.buffer == null)
    {
      if(this.fetch_buffer == null && this.pending_post==false)
      { 
//        console.log("initial fetch");     
        this.fetch_data();
      }
      if(this.fetch_buffer!= null)
      {
//        console.log("buffer=fetch_buffer;");
        this.buffer=this.fetch_buffer;
        this.fetch_buffer=null;
        this.buf_addr=0;
      }
    }
    if(this.buffer != null && this.buf_addr>=this.buffer.length/4)
    {
      if(this.fetch_buffer==null && this.pending_post==false) 
      {       
//        console.log("buf address pointer="+ buf_addr)
        this.fetch_data();
      }
    }
    if(this.buffer!=null)
    {
      const output = outputs[0];

      let pos=this.buf_addr;
      output[0].set(this.buffer.subarray(pos,pos+128));
      pos+=4096;//this.buffer.length/2;
      output[1].set(this.buffer.subarray(pos,pos+128));
      this.buf_addr+=128;

 
      if(this.buf_addr>=this.buffer.length/2)
      {
//        console.log("buffer empty. fetch_buffer ready="+(fetch_buffer!= null));
        this.recyle_buffer = this.buffer;
        this.buffer=null;
        this.buf_addr=0;
      }
    }
/*    else
    {
//      console.error("audio processor has no data");
    }
*/
    return true;
  }
}

registerProcessor('vAmiga_audioprocessor', vAmigaAudioProcessor);
