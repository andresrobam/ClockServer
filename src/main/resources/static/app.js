var app = angular.module("restapp", []);
app.controller("appcontroller",
	function($scope, $http) {

		$scope.getstate = function() {
			$http({
				method: "GET",
				url: "getstate"
			}).success(function(data) {

				$scope.state = data;
			});
		};

		$scope.getstate();

		$scope.sendtoggle = function() {
			$http({
				method: "GET",
				url: "toggle"
			});
		};

		$scope.sendgoto = function() {
			$http({
				method: "GET",
				url: "goto",
				params: {
					x: $scope.state.gotox,
					y: $scope.state.gotoy
				}
			});
		};

		$scope.sendoffset = function() {
			$http({
				method: "GET",
				url: "offset",
				params: {
					x: $scope.state.offsetx,
					y: $scope.state.offsety
				}
			});
		};

		$scope.senddraw = function() {
			$http({
				method: "GET",
				url: "draw",
				params: {
					message: $scope.draw
				}
			});
		};

		$scope.sendoptions = function() {
			$http({
				method: "GET",
				url: "setoptions",
				params: {
					bright: $scope.state.bright,
					dotting: $scope.state.dotting,
					dist: $scope.state.dist,
					sqr: $scope.state.sqr,
					sep: $scope.state.sep,
					sped: $scope.state.sped,
					motordelay: $scope.state.motordelay,
					dotdelay: $scope.state.dotdelay,
					requestdelay: $scope.state.requestdelay
				}
			});
		};

		$scope.defaultoptions = function() {
			if (confirm("Are you sure you want to restore the default settings?")) {
				$http({
					method: "GET",
					url: "defaultoptions"
				}).success(function(data) {

					$scope.state = data;
				});
			}
		};

		$scope.sendclock = function() {
			$http({
				method: "GET",
				url: "clock",
				params: {
					on: $scope.state.clock
				}
			});
		};

		$scope.drawclock = function() {
			$http({
				method: "GET",
				url: "drawclock"
			});
		};

	});
