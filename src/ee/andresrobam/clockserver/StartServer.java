package ee.andresrobam.clockserver;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;

@SpringBootApplication
public class StartServer {
	
	public static LaserState laserstate = new LaserState();

    public static void main(String[] args) throws Exception {
    	
        SpringApplication.run(StartServer.class, args);
    }
}