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
      this.recyle_buffer=new Float32Array(8192);
    }
    this.port.postMessage(this.recyle_buffer, [this.recyle_buffer.buffer]);
    this.recyle_buffer=null;
  }


  process(inputs, outputs) {
    if(this.buffer == null)
    {
      if(this.fetch_buffer!= null)
      {
//        console.log("buffer=fetch_buffer;");
        this.buffer=this.fetch_buffer;
        this.fetch_buffer=null;
        this.buf_addr=0;
      }
      else if(this.pending_post==false)
      { 
//        console.log("initial fetch");     
        this.fetch_data();
      }
    }
    else
    {
      const output = outputs[0];

      let startpos=this.buf_addr;
      let endpos=startpos+128;
      output[0].set(this.buffer.subarray(startpos,endpos));
      output[1].set(this.buffer.subarray(4096+startpos,4096+endpos));
      this.buf_addr=endpos;
      
      if(endpos>=2048 /*this.buffer.length/4*/)
      {
        if(endpos>=4096) //this.buffer.length/2
        {
  //        console.log("buffer empty. fetch_buffer ready="+(fetch_buffer!= null));
          this.recyle_buffer = this.buffer;
          this.buffer=null;
          this.buf_addr=0;
        }  
        if(this.fetch_buffer==null && this.pending_post==false) 
        {       
  //        console.log("buf address pointer="+ buf_addr)
          this.fetch_data();      
        }
      }
  
    }

    return true;
  }
}

registerProcessor('vAmiga_audioprocessor', vAmigaAudioProcessor);
