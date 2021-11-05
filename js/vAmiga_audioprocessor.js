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
/*    this.samples_processed=0;
    this.time=Date.now();
    this.no_data=0;
*/
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
/*    if(this.samples_processed>=44100*30)
    {
      const d= Date.now();
      const millis = d - this.time;
      console.log("ap_samples_processed 44.1khz in "+millis+"ms, no_data="+this.no_data);
      this.samples_processed=0;
      this.no_data=0;
      this.time=d;
    }
    this.samples_processed+=128;
*/
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
    if(this.buffer!=null)
    {
      const output = outputs[0];

      let startpos=this.buf_addr;
      let endpos=startpos+128;
      output[0].set(this.buffer.subarray(startpos,endpos));
      output[1].set(this.buffer.subarray(4096+startpos,4096+endpos));
      this.buf_addr=endpos;
      
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
/*    else
    {
     // console.log("no data");
      this.no_data++;
    }
*/
    return true;
  }
}

registerProcessor('vAmiga_audioprocessor', vAmigaAudioProcessor);
