#include <idynutils/idynutils.h>
#include <kdl/frames.hpp>
#include <OpenSoT/utils/AutoStack.h>
#include <OpenSoT/utils/DefaultHumanoidStack.h>
#include <OpenSoT/utils/Previewer.h>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/rolling_mean.hpp>

/**
 * @brief The MyTrajGen class dummy trajectory generator:
 *        it creates a constant trajectory that lasts one second.
 *        The goal pose is with p={0,0,0}, R=I
 */
class MyTrajGen
{
public:
    typedef boost::shared_ptr<MyTrajGen> Ptr;
    KDL::Frame Pos(double time) { return KDL::Frame(); }
    KDL::Twist Vel(double time) { return KDL::Twist(); }
    double Duration() { return 1.0; }
};

typedef OpenSoT::Previewer<MyTrajGen> Previewer;
typedef boost::accumulators::accumulator_set<double,
                                            boost::accumulators::stats<boost::accumulators::tag::rolling_mean>
                                            > Accumulator;

int main(int argc, char **argv) {

    const double dT = 3e-3;
    iDynUtils _robot("coman",
                     std::string(OPENSOT_TESTS_ROBOTS_DIR)+"coman/coman.urdf",
                     std::string(OPENSOT_TESTS_ROBOTS_DIR)+"coman/coman.srdf");
    yarp::sig::Vector q = _robot.iDyn3_model.getAng();

    OpenSoT::DefaultHumanoidStack DHS(_robot, dT, _robot.zeros);

    // defining a stack composed of size two,
    // where the task of first priority is an aggregated of leftArm and rightArm,
    // rightArm contains a convexHull constraint;
    // the task at the second priority level is an aggregated of rightLeg and leftLeg,
    // and the stack is subject to bounds jointLimits and velocityLimits
    OpenSoT::AutoStack::Ptr autoStack = 
        ((DHS.leftArm + (DHS.rightArm << DHS.convexHull))
        / (DHS.rightLeg + DHS.leftLeg)) << DHS.jointLimits << DHS.velocityLimits;

    MyTrajGen::Ptr trajLeftArm(new MyTrajGen());
    MyTrajGen::Ptr trajRightArm(new MyTrajGen());

    Previewer::TrajectoryBindings bindings;
    bindings.push_back(Previewer::TrajBinding(trajLeftArm, DHS.leftArm));
    bindings.push_back(Previewer::TrajBinding(trajRightArm, DHS.rightArm));

    Previewer::Ptr previewer(new Previewer(dT, _robot, autoStack, bindings));

    Previewer::Results results;
    previewer->check(1.0,3,&results);

    std::cout << "Logged " << results.failures.size() << " failures:" << std::endl;
    for(unsigned int i = 0; i < results.failures.size(); ++i)
    {
        std::cout << "@t:" << results.failures[i].t
                  << " - " << Previewer::Results::reasonToString(results.failures[i].reason)
                  << std::endl;
    }

    std::cout << "Logged " << results.trajectory.size() << " trajectory nodes:" << std::endl;
    for(unsigned int i = 0; i < results.trajectory.size(); ++i)
    {
        std::cout << "@t:" << results.trajectory[i].t
                  << " - " << results.trajectory[i].q.toString()
                  << std::endl;
    }
}
