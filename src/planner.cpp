#include <pluginlib/class_list_macros.h>
#include "smooth_planner/planner.h"


PLUGINLIB_EXPORT_CLASS(smooth_planner::SmoothPlanner, nav_core::BaseGlobalPlanner)

using namespace std;

namespace smooth_planner
{

	SmoothPlanner::SmoothPlanner()
	{

	}

	SmoothPlanner::SmoothPlanner(std::string name, costmap_2d::Costmap2DROS* costmap_ros)
	{
		
		initialize(name, costmap_ros);

		max_iterations = 20000;



	}

	SmoothPlanner::~SmoothPlanner()
	{
		delete reconfigure_server;
	}

	void SmoothPlanner::initialize(std::string name, costmap_2d::Costmap2DROS* costmap_ros)
	{
		ros::NodeHandle nh;
		costmap_ = costmap_ros->getCostmap();

		marker_pub = nh.advertise<visualization_msgs::Marker>("tree_marker", 10);

		plan_pub_ = nh.advertise<nav_msgs::Path>("plan", 10);

		marker_pub2 = nh.advertise<visualization_msgs::Marker>("tree_marker2", 10);

		plan_pub_2 = nh.advertise<nav_msgs::Path>("plan2", 10);

		tree.initialize(costmap_);

		max_iterations = 20000;
		ros::NodeHandle node_private("~/" + name);
		dynamic_reconfigure::Server<smooth_planner::SmoothPlannerConfig>::CallbackType cb;
		reconfigure_server = new dynamic_reconfigure::Server<smooth_planner::SmoothPlannerConfig>(node_private);
		cb = boost::bind(&SmoothPlanner::dynamic_reconfigure_callback, this, _1, _2);
		reconfigure_server->setCallback(cb);


	}

	void SmoothPlanner::dynamic_reconfigure_callback(smooth_planner::SmoothPlannerConfig &config, uint32_t level)
	{
		boost::recursive_mutex::scoped_lock cfl(configuration_mutex_);

		max_iterations = config.max_iterations;

		tree.set_wall_clearance(config.wall_clearance);
		tree.set_goal_threshold(config.goal_thresh_trans, config.goal_thresh_rot);
		tree.set_gains(config.pursue_gain, config.angle_gain, config.guide_gain);
		tree.set_grid_resolution(config.grid_resolution_xy, config.grid_resolution_theta);
		tree.set_zig_zag_cost(config.zig_zag_cost);

	}

	bool SmoothPlanner::makePlan(
		const geometry_msgs::PoseStamped& start, 
		const geometry_msgs::PoseStamped &goal, 
		std::vector<geometry_msgs::PoseStamped>& plan )
	{


		ros::Time begin = ros::Time::now();


		tree.init_starting_pose(start.pose);
		
		tree.grid_astar(start.pose, goal.pose);

		int best_start_to_end ;
		
		int lim = 0;
		char stat = 0;
		while(lim++<max_iterations )
		{
			
			best_start_to_end = tree.find_best_end();
			int v = tree.expand_node(best_start_to_end, goal);
			
			if(v == -1 ) {
				stat = -1;
				break;
			}
			else if(v == 0) continue;
			else if(v == 1)
			{
				stat = 1;
				break;
			}
			else if(v == 2)
			{
				stat = 2;
			}
		}

		tree.create_road_to_final(plan);

		//std::reverse(plan.begin(), plan.end());



		nav_msgs::Path gui_path;
		gui_path.poses.resize(plan.size());

		gui_path.header.frame_id = "/map";
		gui_path.header.stamp = ros::Time::now();

		for (unsigned int i = 0; i < plan.size(); i++) {
			gui_path.poses[i] = plan[i];
		}

		plan_pub_.publish(gui_path);


		nav_msgs::Path gui_path2;
		gui_path2.poses.resize(tree.grid_road.size());

		gui_path2.header.frame_id = "/map";
		gui_path2.header.stamp = ros::Time::now();

		for (unsigned int i = 0; i < tree.grid_road.size(); i++) {
			gui_path2.poses[i].header.stamp = ros::Time::now();
			gui_path2.poses[i].header.frame_id = "/map";
			gui_path2.poses[i].pose.position.x = tree.grid_road[i].first;
			gui_path2.poses[i].pose.position.y = tree.grid_road[i].second;
			gui_path2.poses[i].pose.position.z = 0;
			gui_path2.poses[i].pose.orientation.x = 0;
			gui_path2.poses[i].pose.orientation.y = 0;
			gui_path2.poses[i].pose.orientation.z = 0;
			gui_path2.poses[i].pose.orientation.w = 1;


		}

		plan_pub_2.publish(gui_path2);
		
		visualization_msgs::Marker tree_marker;
		tree_marker.header.frame_id = "/map";
		tree_marker.type = visualization_msgs::Marker::LINE_LIST;
		tree_marker.scale.x = 0.002;
		tree_marker.color.b = 1.0f;
		tree_marker.color.r = 1.0f;
		tree_marker.color.a = 0.7f;

		tree.create_line_list(tree_marker);
		
		marker_pub.publish(tree_marker);


/*
		Tree t2;
		t2.init_starting_pose(start.pose);

		t2.costmap_ = costmap_;
		
		


		lim = 0;
		while(lim++<50000 && t2.expand_best_end(goal, plan) );

		

		



		t2.create_line_list(tree_marker2);

		*/
		
		ros::Time end = ros::Time::now();
		ros::Duration elapsed = end - begin;
		std::cout<< "Elapsed time: " << elapsed.toSec() << "\n";


		if(stat == -1 || stat == 0) return false;
		if(stat == 2) return true;
		else return true;
		

	}



};