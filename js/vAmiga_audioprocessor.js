let fetch_buffer=null;
let buffer=null;
let buf_addr=0;
let pending_post=false;

class vAmigaAudioProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.port.onmessage = this.handleMessage.bind(this);
    this.port.onmessageerror = (error)=>{ 
      console.error("error:"+ error);
    };
    console.log("vAmiga_audioprocessor connected");
  }

  handleMessage(event) {
  //  console.log("processor received sound data");
    fetch_buffer = event.data;
    pending_post=false;
  }

  fetch_data()
  {
//    console.log("audio processor: fetch");
    pending_post=true;
    this.port.postMessage("fetch");
  }


  process(inputs, outputs) {
    if(buffer == null)
    {
      if(fetch_buffer == null && pending_post==false)
      { 
//        console.log("initial fetch");     
        this.fetch_data();
      }
      if(fetch_buffer!= null)
      {
//        console.log("buffer=fetch_buffer;");
        buffer=fetch_buffer;
        fetch_buffer=null;
        buf_addr=0;
      }
    }
    if(buffer != null && buf_addr>=buffer.length/2)
    {
      if(fetch_buffer==null && pending_post==false) 
      {       
//        console.log("buf address pointer="+ buf_addr)
        this.fetch_data();
      }
    }
    if(buffer!=null)
    {
      const output = outputs[0];
      for(let i=0;i<128;i++)
      {
        output[0][i]=buffer[buf_addr++];
        output[1][i]=buffer[buf_addr++];
      }
 
      if(buf_addr>=buffer.length)
      {
//        console.log("buffer empty. fetch_buffer ready="+(fetch_buffer!= null));
        buffer=null;
        buf_addr=0;
      }
    }
    else
    {
//      console.error("audio processor has no data");
    }
    return true;
  }
}

registerProcessor('vAmiga_audioprocessor', vAmigaAudioProcessor);
