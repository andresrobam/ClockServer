package main.java.clockserver;

import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

@RestController
public class Handler {
	
	LaserState laser = StartServer.laserstate;
	
	@RequestMapping(value = "/getstate")
	public LaserState getstate() {	//gets all the parameters of the laser (resets some of them after the request)
		
		if (laser.isClock())	//checks if clock mode is activated
			laser.calcTime();	//if it is, figures out whether it has to draw
		
		return laser;
	}

	@RequestMapping(value = "/toggle")
	public void laser() {
			
		laser.setOn(!laser.isOn());
	}
	
	@RequestMapping(value = "/goto")
	public void gotocoords(
			@RequestParam(value = "x", defaultValue = "0") int x,
			@RequestParam(value = "y", defaultValue = "0") int y
			) {
			
		laser.setGotox(x);
		laser.setGotoy(y);
	}
	
	@RequestMapping(value = "/offset")
	public void offset(
			@RequestParam(value = "x", defaultValue = "0") int x,
			@RequestParam(value = "y", defaultValue = "0") int y
			) {
			
		laser.setOffsetx(x);
		laser.setOffsety(y);
	}
	
	@RequestMapping(value = "/draw")
	public void draw(
			@RequestParam(value = "message", defaultValue = "0000") String message
			) {
		
		boolean work = true;
		
		if (message.length() == 4) {		//drawn message has to have a length of 4
			
			try {
				Integer.parseInt(message);	//has to contain only numbers
				laser.setDigits(message);
			}
			catch (NumberFormatException e) {}
		}
		else if (message.equals("borders")) {	//you can also enter "borders" to the frontend to draw digit boundaries
			laser.setAllDigits("96");
		}
		else
			work = false;
		laser.setWork(work);
	}
	
	@RequestMapping(value = "/setoptions")
	public void setoptions(
			@RequestParam(value = "bright", required = true) int bright,
			@RequestParam(value = "dotting", required = true) boolean dotting,
			@RequestParam(value = "dist", required = true) int dist,
			@RequestParam(value = "sqr", required = true) int sqr,
			@RequestParam(value = "sep", required = true) int sep,
			@RequestParam(value = "sped", required = true) float sped,
			@RequestParam(value = "motordelay", required = true) int motordelay,
			@RequestParam(value = "dotdelay", required = true) int dotdelay,
			@RequestParam(value = "requestdelay", required = true) int requestdelay
			) {
		
		if (bright < 0)
			bright = 0;
		else if (bright > 1023)
			bright = 1023;
		laser.setBright(bright);
		laser.setDotting(dotting);
		if (dist > 0)
			laser.setDist(dist);
		if (sqr > 0)
			laser.setSqr(sqr);
		if (sep > 0)
			laser.setSep(sep);
		if (!(sped <= 0))
			laser.setSped(sped);
		if (motordelay > 0)
			laser.setMotordelay(motordelay);
		if (dotdelay > 0)
			laser.setDotdelay(dotdelay);
		if (requestdelay > 0)
			laser.setRequestdelay(requestdelay);
	}
	
	@RequestMapping(value = "/clock")
	public void clock(
			@RequestParam(value = "on", defaultValue = "true") boolean on
			) {
		
		laser.setClock(on);
	}
	
	@RequestMapping(value = "/drawclock")
	public void drawclock() {
		
		laser.setDigits(LaserState.getTime());
		laser.setWork(true);
	}
	
	@RequestMapping(value = "/defaultoptions")
	public LaserState defaultoptions() {
		
		laser = new LaserState();
		return getstate();
	}
}