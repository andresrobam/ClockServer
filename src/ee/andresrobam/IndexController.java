package ee.andresrobam;

import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;

@Controller
public class IndexController {
	
	@RequestMapping("/")		//direct requests coming to localhost:8080 to localhost:8080/index
    public String getSlash() {
        return "index";
    }
}