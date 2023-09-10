var VirtualJoystick	= function(opts)
{
	opts			= opts			|| {};
	this._container		= opts.container	|| document.body;
	this._strokeStyle	= opts.strokeStyle	|| 'cyan';
	this._stickEl		= opts.stickElement	|| this._buildJoystickStick();
	this._baseEl		= opts.baseElement	|| this._buildJoystickBase();
	this._mouseSupport	= opts.mouseSupport !== undefined ? opts.mouseSupport : false;
	this._stationaryBase	= opts.stationaryBase || false;
	this._baseX		= this._stickX = opts.baseX || 0
	this._baseY		= this._stickY = opts.baseY || 0
	this._limitStickTravel	= opts.limitStickTravel || false
	this._stickRadius	= opts.stickRadius !== undefined ? opts.stickRadius : 100
	//this._container.style.position	= "relative"

	this._container.appendChild(this._baseEl)
	this._baseEl.style.position	= "absolute"
	this._baseEl.style.display	= "none"
	this._container.appendChild(this._stickEl)
	this._stickEl.style.position	= "absolute"
	this._stickEl.style.display	= "none"

	this._pressed	= false;
	this._touchIdx	= null;
	
	if(this._stationaryBase === true){
		this._baseEl.style.display	= "";
		this._baseX=this._baseEl.width /2;

		this._baseEl.style.left		= (this._baseX - this._baseEl.width /2 +10)+"px";		
		let middle=current_vjoy_touch.includes("middle");
		this._baseEl.style.top=
		 `calc(${middle?50:90}vh - ${(middle?this._baseEl.height/2:this._baseEl.height) +10}px)`
		this._baseY=this._baseEl.offsetTop+this._baseEl.height/2;
	
	}
    	
	var __bind	= function(fn, me){ return function(){ return fn.apply(me, arguments); }; };
	this._$onTouchStart	= __bind(this._onTouchStart	, this);
	this._$onTouchEnd	= __bind(this._onTouchEnd	, this);
	this._$onTouchMove	= __bind(this._onTouchMove	, this);
	this._container.addEventListener( 'touchstart'	, this._$onTouchStart	, false );
	this._container.addEventListener( 'touchend'	, this._$onTouchEnd	, false );
	this._container.addEventListener( 'touchmove'	, this._$onTouchMove	, false );
	if( this._mouseSupport ){
		this._$onMouseDown	= __bind(this._onMouseDown	, this);
		this._$onMouseUp	= __bind(this._onMouseUp	, this);
		this._$onMouseMove	= __bind(this._onMouseMove	, this);
		this._container.addEventListener( 'mousedown'	, this._$onMouseDown	, false );
		this._container.addEventListener( 'mouseup'	, this._$onMouseUp	, false );
		this._container.addEventListener( 'mousemove'	, this._$onMouseMove	, false );
	}
}

VirtualJoystick.prototype.destroy	= function()
{
	this._container.removeChild(this._baseEl);
	this._container.removeChild(this._stickEl);

	this._container.removeEventListener( 'touchstart'	, this._$onTouchStart	, false );
	this._container.removeEventListener( 'touchend'		, this._$onTouchEnd	, false );
	this._container.removeEventListener( 'touchmove'	, this._$onTouchMove	, false );
	if( this._mouseSupport ){
		this._container.removeEventListener( 'mouseup'		, this._$onMouseUp	, false );
		this._container.removeEventListener( 'mousedown'	, this._$onMouseDown	, false );
		this._container.removeEventListener( 'mousemove'	, this._$onMouseMove	, false );
	}
}

/**
 * @returns {Boolean} true if touchscreen is currently available, false otherwise
*/
VirtualJoystick.touchScreenAvailable	= function()
{
	return 'createTouch' in document ? true : false;
}

/**
 * microevents.js - https://github.com/jeromeetienne/microevent.js
*/
;(function(destObj){
	destObj.addEventListener	= function(event, fct){
		if(this._events === undefined) 	this._events	= {};
		this._events[event] = this._events[event]	|| [];
		this._events[event].push(fct);
		return fct;
	};
	destObj.removeEventListener	= function(event, fct){
		if(this._events === undefined) 	this._events	= {};
		if( event in this._events === false  )	return;
		this._events[event].splice(this._events[event].indexOf(fct), 1);
	};
	destObj.dispatchEvent		= function(event /* , args... */){
		if(this._events === undefined) 	this._events	= {};
		if( this._events[event] === undefined )	return;
		var tmpArray	= this._events[event].slice(); 
		for(var i = 0; i < tmpArray.length; i++){
			var result	= tmpArray[i].apply(this, Array.prototype.slice.call(arguments, 1))
			if( result !== undefined )	return result;
		}
		return undefined
	};
})(VirtualJoystick.prototype);

//////////////////////////////////////////////////////////////////////////////////
//										//
//////////////////////////////////////////////////////////////////////////////////

VirtualJoystick.prototype.deltaX	= function(){ return this._stickX - this._baseX;	}
VirtualJoystick.prototype.deltaY	= function(){ return this._stickY - this._baseY;	}
rest_zone=12;
VirtualJoystick.prototype.up	= function(){
	if( this._pressed === false )	return false;
	//var deltaX	= this.deltaX();
	var deltaY	= this.deltaY();
	if( deltaY >= 0 )				return false;
	//if( Math.abs(deltaX) > 2*Math.abs(deltaY) )	return false;
	return Math.abs(deltaY)>rest_zone;
}
VirtualJoystick.prototype.down	= function(){
	if( this._pressed === false )	return false;
	//var deltaX	= this.deltaX();
	var deltaY	= this.deltaY();
	if( deltaY <= 0 )				return false;
	//if( Math.abs(deltaX) > 2*Math.abs(deltaY) )	return false;
	return Math.abs(deltaY)>rest_zone;
}
VirtualJoystick.prototype.right	= function(){
	if( this._pressed === false )	return false;
	var deltaX	= this.deltaX();
	//var deltaY	= this.deltaY();
	if( deltaX <= 0 )				return false;
	//if( Math.abs(deltaY) > 2*Math.abs(deltaX) )	return false;
	return Math.abs(deltaX)>rest_zone;	
}
VirtualJoystick.prototype.left	= function(){
	if( this._pressed === false )	return false;
	var deltaX	= this.deltaX();
	//var deltaY	= this.deltaY();
	if( deltaX >= 0 )				return false;
	//if( Math.abs(deltaY) > 2*Math.abs(deltaX) )	return false;
	return Math.abs(deltaX)>rest_zone;
}

//////////////////////////////////////////////////////////////////////////////////
//										//
//////////////////////////////////////////////////////////////////////////////////

VirtualJoystick.prototype._onUp	= function()
{
	this._pressed	= false; 
	this._stickEl.style.display	= "none";
	
	if(this._stationaryBase == false){	
		this._baseEl.style.display	= "none";
	
		this._baseX	= this._baseY	= 0;
		this._stickX	= this._stickY	= 0;
	}
}

VirtualJoystick.prototype._onDown	= function(x, y)
{
	this._pressed	= true; 
	if(this._stationaryBase == false){
		this._baseX	= x;
		this._baseY	= y;
		this._baseEl.style.display	= "";
		this._move(this._baseEl.style, (this._baseX - this._baseEl.width /2), (this._baseY - this._baseEl.height/2));
	}
	
	this._stickX	= x;
	this._stickY	= y;
	
	if(this._limitStickTravel === true){
		var deltaX	= this.deltaX();
		var deltaY	= this.deltaY();
		var stickDistance = Math.sqrt( (deltaX * deltaX) + (deltaY * deltaY) );
		if(stickDistance > this._stickRadius){
			var stickNormalizedX = deltaX / stickDistance;
			var stickNormalizedY = deltaY / stickDistance;
			
			this._stickX = stickNormalizedX * this._stickRadius + this._baseX;
			this._stickY = stickNormalizedY * this._stickRadius + this._baseY;
		} 	
	}
	
	this._stickEl.style.display	= "";
	this._move(this._stickEl.style, (this._stickX - this._stickEl.width /2), (this._stickY - this._stickEl.height/2));	
}

VirtualJoystick.prototype._onMove	= function(x, y)
{
	if( this._pressed === true ){
		this._stickX	= x;
		this._stickY	= y;

		if(this._limitStickTravel === true){
			var deltaX	= this.deltaX();
			var deltaY	= this.deltaY();
			var stickDistance = Math.sqrt( (deltaX * deltaX) + (deltaY * deltaY) );
			if(stickDistance > this._stickRadius){
				var stickNormalizedX = deltaX / stickDistance;
				var stickNormalizedY = deltaY / stickDistance;
			
				this._stickX = stickNormalizedX * this._stickRadius + this._baseX;
				this._stickY = stickNormalizedY * this._stickRadius + this._baseY;
			} 		

			//vc64web patch start let the base move too, when innercircle collides with outercircle 
			if(this._stationaryBase)
			{
				this._baseY=this._baseEl.offsetTop+this._baseEl.height/2;
			}
			else
			{
				var base_radius = this._baseEl.width/2;
				if(stickDistance >= base_radius/2){
					this._baseX	= this._stickX - ((this._stickX - this._baseX)/stickDistance)*base_radius/2; 
					this._baseY	= this._stickY - ((this._stickY - this._baseY)/stickDistance)*base_radius/2;
					this._baseEl.style.display	= "";
					if(!fixed_touch_joystick_base)
						this._move(this._baseEl.style, (this._baseX - base_radius), (this._baseY - base_radius));	
				}
			}
			//vc64web patch end
		}
		
        this._move(this._stickEl.style, (this._stickX - this._stickEl.width /2), (this._stickY - this._stickEl.height/2));	
	}	
}


//////////////////////////////////////////////////////////////////////////////////
//		bind touch events (and mouse events for debug)			//
//////////////////////////////////////////////////////////////////////////////////

VirtualJoystick.prototype._onMouseUp	= function(event)
{
	return this._onUp();
}

VirtualJoystick.prototype._onMouseDown	= function(event)
{
	var isValid	= this.dispatchEvent('touchStartValidation', event);
	if( isValid === false )	return;

	event.preventDefault();
	var x	= event.clientX;
	var y	= event.clientY;
	return this._onDown(x, y);
}

VirtualJoystick.prototype._onMouseMove	= function(event)
{
	var x	= event.clientX;
	var y	= event.clientY;
	return this._onMove(x, y);
}

//////////////////////////////////////////////////////////////////////////////////
//		comment								//
//////////////////////////////////////////////////////////////////////////////////

VirtualJoystick.prototype._onTouchStart	= function(event)
{
	// if there is already a touch inprogress do nothing
//	if( this._touchIdx !== null )	return;

	if(typeof dragItems !== 'undefined' && dragItems.includes(event.target)) return;

	// notify event for validation
	var isValid	= this.dispatchEvent('touchStartValidation', event);
	if( isValid === false )	return;
	
	// dispatch touchStart
	this.dispatchEvent('touchStart', event);

	event.preventDefault();
	// get the first who changed
	var touch	= event.changedTouches[0];
	// set the touchIdx of this joystick
	this._touchIdx	= touch.identifier;

	// forward the action
	var x		= touch.pageX;
	var y		= touch.pageY;
	return this._onDown(x, y)
}

//touch_check=true;
VirtualJoystick.prototype._onTouchEnd	= function(event)
{
	// if there is no touch in progress, do nothing
//	if( touch_check && this._touchIdx === null )	return;

	// dispatch touchEnd
	this.dispatchEvent('touchEnd', event);

	// try to find our touch event
	var touchList	= event.changedTouches;
	for(var i = 0; i < touchList.length && touchList[i].identifier !== this._touchIdx; i++);
	// if touch event isnt found, 
	if( i === touchList.length)	return;

	// reset touchIdx - mark it as no-touch-in-progress
	this._touchIdx	= null;

	event.preventDefault();

	return this._onUp()
}

VirtualJoystick.prototype._onTouchMove	= function(event)
{
	// if there is no touch in progress, do nothing
//	if( touch_check && this._touchIdx === null )	return;

	// try to find our touch event
	var touchList	= event.changedTouches;
	for(var i = 0; i < touchList.length && touchList[i].identifier !== this._touchIdx; i++ );
	// if touch event with the proper identifier isnt found, do nothing
	if( i === touchList.length)	return;
	var touch	= touchList[i];

	event.preventDefault();

	var x		= touch.pageX;
	var y		= touch.pageY;
	return this._onMove(x, y)
}


//////////////////////////////////////////////////////////////////////////////////
//		build default stickEl and baseEl				//
//////////////////////////////////////////////////////////////////////////////////

/**
 * build the canvas for joystick base
 */
VirtualJoystick.prototype._buildJoystickBase	= function()
{
	var canvas	= document.createElement( 'canvas' );
	canvas.width	= 148;
	canvas.height	= 148;
	this._drawJoystickBase(canvas);	
	return canvas;
}

VirtualJoystick.prototype._drawJoystickBase	= function(canvas)
{
	var ctx		= canvas.getContext('2d');	
	if(this._strokeStyle=='white')
	{
		ctx.beginPath();
		ctx.strokeStyle="rgba(80, 80, 80, 0.5)";
		ctx.lineWidth	= 28; 
		ctx.arc(148/2, 148/2, 60, 0, 2*Math.PI);
		ctx.stroke();
		
		ctx.globalCompositeOperation = "destination-out";
		ctx.lineWidth	= 1; 
		let path = new Path2D();
		path.moveTo(5,0);
		path.lineTo(148,148-5);
		path.lineTo(148-5,148);
		path.lineTo(0,5);
		path.lineTo(5,0);
		path.moveTo(148-5,0);
		path.lineTo(0,148-5);
		path.lineTo(5,148);
		path.lineTo(148,5);
		path.lineTo(148-55,0);
		
		ctx.fillStyle="rgba(0, 0, 0, 1.0)";
		ctx.fill(path);	
		ctx.globalCompositeOperation = 'source-over';
	}
	else
	{
		ctx.beginPath();
		ctx.strokeStyle="rgba(255, 0, 0, 0.25)";
		ctx.lineWidth	= 26; 
		ctx.arc(148/2, 148/2, 48, 0, 2*Math.PI);
		ctx.stroke();
	}
}

VirtualJoystick.prototype.redraw_base= function(cmd)
{
	if(this._strokeStyle=='white')
	{
		let ctx=this._baseEl.getContext('2d');	
		ctx.clearRect(0,0,this._baseEl.width,this._baseEl.height);
		this._drawJoystickBase(this._baseEl);

		ctx.strokeStyle = this._strokeStyle; 
		let width=this._baseEl.width;
		let height=this._baseEl.height;
		let m=width/2;
		let w=8;
		let h=8;
		let border=10;

		ctx.fillStyle="white"
		ctx.lineWidth=3;

		ctx.beginPath();

		if(cmd.includes("UP"))
		{
			ctx.moveTo(m,border);
			ctx.lineTo(m+w,border+h);
			ctx.moveTo(m-w,border+h);
			ctx.lineTo(m,border);
		}
		else if(cmd.includes("DOWN"))
		{
			ctx.moveTo(m,height-border);
			ctx.lineTo(m+w,height-border-h);
			ctx.moveTo(m-w,height-border-h);
			ctx.lineTo(m,height-border);
		}
		if(cmd.includes("LEFT"))
		{
			ctx.moveTo(border,m);
			ctx.lineTo(border+h,m+w);
			ctx.moveTo(border+h,m-w);
			ctx.lineTo(border,m);
		} 
		else if(cmd.includes("RIGHT"))
		{
			ctx.moveTo(width-border,m);
			ctx.lineTo(width-border-h,m+w);
			ctx.moveTo(width-border-h,m-w);
			ctx.lineTo(width-border,m);
		}
		ctx.stroke();
	}
}

/**
 * build the canvas for joystick stick
 */
VirtualJoystick.prototype._buildJoystickStick	= function()
{
	var canvas	= document.createElement( 'canvas' );
	canvas.width	= 92;
	canvas.height	= 92;
	var ctx		= canvas.getContext('2d');
	
	if(this._strokeStyle=='white')
	{
		ctx.beginPath(); 
		ctx.strokeStyle	= "rgba(180,180,180,0.2)"//this._strokeStyle; 
		ctx.lineWidth	= 10; 
		ctx.arc( canvas.width/2, canvas.width/2, 38, 0, Math.PI*2, true); 
		ctx.stroke();
	}
	else
	{
	}
	return canvas;
}

VirtualJoystick.prototype._move = function(style, x, y)
{
	style["transform"] = 'translate3d(' + x + 'px,' + y + 'px, 0)';
}