package main.java.clockserver;

import java.time.LocalDateTime;

public class LaserState {
	
	boolean dotting = true;		//fields
	int bright = 1023;
	int dist = 300;
	int sqr = 30;
	int sep = 6;
	boolean on = false;
	int offsetx = 0;
	int offsety = 0;
	int gotox = 0;
	int gotoy = 0;
	int motordelay = 4;
	int requestdelay = 500;
	int dotdelay = 100;
	String digit1 = "0";
	String digit2 = "0";
	String digit3 = "0";
	String digit4 = "0";
	boolean work = false;
	float sped = 0.015f;
	boolean clock = false;
	String oldtime = "";
	
	public LaserState(){}

	public boolean isDotting() {
		return dotting;
	}

	public void setDotting(boolean dotting) {
		this.dotting = dotting;
	}

	public int getBright() {
		return bright;
	}

	public void setBright(int bright) {
		this.bright = bright;
	}

	public int getDist() {
		return dist;
	}

	public void setDist(int dist) {
		this.dist = dist;
	}

	public int getSqr() {
		return sqr;
	}

	public void setSqr(int sqr) {
		this.sqr = sqr;
	}

	public int getSep() {
		return sep;
	}

	public void setSep(int sep) {
		this.sep = sep;
	}

	public boolean isOn() {
		return on;
	}

	public void setOn(boolean on) {
		this.on = on;
	}

	public int getOffsetx() {		//resets after laser gets the values, this way the laser wont repeat the action immediately
		int oldoffsetx = offsetx;
		offsetx = 0;
		return oldoffsetx;
	}

	public void setOffsetx(int offsetx) {
		this.offsetx = offsetx;
	}

	public int getOffsety() {		//resets after laser gets the values, this way the laser wont repeat the action immediately
		int oldoffsety = offsety;
		offsety = 0;
		return oldoffsety;
	}

	public void setOffsety(int offsety) {
		this.offsety = offsety;
	}

	public int getGotox() {
		return gotox;
	}

	public void setGotox(int gotox) {
		this.gotox = gotox;
	}

	public int getGotoy() {
		return gotoy;
	}

	public void setGotoy(int gotoy) {
		this.gotoy = gotoy;
	}
	
	public int getMotordelay() {
		return motordelay;
	}

	public void setMotordelay(int motordelay) {
		this.motordelay = motordelay;
	}

	public int getRequestdelay() {
		return requestdelay;
	}

	public void setRequestdelay(int requestdelay) {
		this.requestdelay = requestdelay;
	}

	public int getDotdelay() {
		return dotdelay;
	}

	public void setDotdelay(int dotdelay) {
		this.dotdelay = dotdelay;
	}

	public void setDigits(String digits) {
		
		digit1 = digits.substring(0, 1);
		digit2 = digits.substring(1, 2);
		digit3 = digits.substring(2, 3);
		digit4 = digits.substring(3, 4);
	}

	public String getDigit1() {
		return digit1;
	}

	public void setDigit1(String digit1) {
		this.digit1 = digit1;
	}

	public String getDigit2() {
		return digit2;
	}

	public void setDigit2(String digit2) {
		this.digit2 = digit2;
	}

	public String getDigit3() {
		return digit3;
	}

	public void setDigit3(String digit3) {
		this.digit3 = digit3;
	}

	public String getDigit4() {
		return digit4;
	}

	public void setDigit4(String digit4) {
		this.digit4 = digit4;
	}

	public boolean isWork() {	//resets after laser gets the values, this way the laser wont repeat the action immediately
		
		boolean oldwork = work;
		if (work) {
			work = false;
			on = false;
			gotox = 0;
			gotoy = 0;
		}
		return oldwork;
	}

	public void setWork(boolean work) {
		this.work = work;
	}

	public void setAllDigits(String digit) {
		digit1 = digit;
		digit2 = digit;
		digit3 = digit;
		digit4 = digit;
	}
	public boolean isClock() {
		return clock;
	}
	public void setClock(boolean clock) {
		this.clock = clock;
	}

	public float getSped() {
		return sped;
	}

	public void setSped(float sped) {
		this.sped = sped;
	}
    
    public static String getTime() {
    	
    	LocalDateTime time = LocalDateTime.now();	//get current time
		
		String hh = "";		//string to keep hours
		String mm = "";		//string to keep minutes
		
		int hours = time.getHour();			//get hours as an integer		
		int minutes = time.getMinute();		//get minutes as an integer
		
		if (hours < 10)		//we want hours as two digits, for example 03 instead of 3
			hh = "0";
		hh += hours;
		if (minutes < 10)	//same for minutes
			mm = "0";
		mm += minutes;
		
		return hh+mm;	//return the time as a string that has a length of 4
    }

	public void calcTime() {	//figures out if the clock has already been drawn this minute and sets it to draw if not
		
		String newtime = getTime();
		
		if (!newtime.equals(oldtime)) {
			
			work = true;
			setDigits(newtime);
			oldtime = newtime;
		}
	}
}