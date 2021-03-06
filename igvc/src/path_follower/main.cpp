#include <ros/ros.h>
#include <igvc_msgs/action_path.h>
#include <igvc_msgs/velocity_pair.h>
#include <mutex>
#include <chrono>

using namespace igvc_msgs;
using namespace std;
using namespace chrono;

action_pathConstPtr path;
mutex path_mutex;

int path_index = 0;
bool path_reset = false;

void newPath(const action_pathConstPtr &msg)
{
  lock_guard<mutex> lock(mutex);
  path = msg;
  path_index = 0;
  path_reset = true;
}

int main(int argc, char** argv)
{

  ros::init(argc, argv, "path_follower");

  time_point<high_resolution_clock> cmd_start_time;

  ros::NodeHandle n;

  ros::Publisher cmd_pub = n.advertise<velocity_pair>("/motors", 1);

  ros::Subscriber path_sub = n.subscribe("path", 1, newPath);

  ros::Rate rate(120);
  while(ros::ok())
  {
    ros::spinOnce();

    { //scope for mutex lock
      lock_guard<mutex> lock(mutex);
      if(path.get() != nullptr)
      {
        if(path_reset)
        {
          cmd_pub.publish(path->actions[path_index]);
          cmd_start_time = high_resolution_clock::now();
          path_reset = false;
        }
        auto elapsed_time = high_resolution_clock::now() -  cmd_start_time;
        if(duration_cast<seconds>(elapsed_time).count() > path->actions[path_index].duration)
        {
            if(path_index < path->actions.size() - 1)
            {
                path_index++;
                cmd_pub.publish(path->actions[path_index]);
                cmd_start_time = high_resolution_clock::now();
            } else {
                velocity_pair vel;
                vel.left_velocity = 0.;
                vel.right_velocity = 0.;
                cmd_pub.publish(vel);
                path.reset();
            }
        }
      }
    }

    rate.sleep();
  }

  return 0;
}
