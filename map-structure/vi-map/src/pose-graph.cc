#include "vi-map/pose-graph.h"

#include <aslam/common/memory.h>
#include <maplab-common/pose_types.h>

#include "vi-map/vertex.h"
#include "vi-map/viwls-edge.h"

namespace vi_map {

void PoseGraph::addVIVertex(
    const pose_graph::VertexId& id,
    const Eigen::Matrix<double, 6, 1>& imu_ba_bw,
    const Eigen::Matrix<double, 2, Eigen::Dynamic>& img_points,
    const Eigen::VectorXd& uncertainties,
    const aslam::VisualFrame::DescriptorsT& descriptors,
    const std::vector<LandmarkId>& observed_landmark_ids,
    const vi_map::MissionId& mission_id, const aslam::FrameId& frame_id,
    int64_t frame_timestamp, const aslam::NCamera::Ptr cameras) {
  pose_graph::Vertex* vertex(
      new vi_map::Vertex(
          id, imu_ba_bw, img_points, uncertainties, descriptors,
          observed_landmark_ids, mission_id, frame_id, frame_timestamp,
          cameras));
  this->addVertex(pose_graph::Vertex::UniquePtr(vertex));
}

//增加一条imu测量的边
    void PoseGraph::addVIEdge(
            const pose_graph::EdgeId& id, const pose_graph::VertexId& from,
            const pose_graph::VertexId& to,
            const Eigen::Matrix<int64_t, 1, Eigen::Dynamic>& imu_timestamps,
            const Eigen::Matrix<double, 6, Eigen::Dynamic>& imu_data)
    {
    //调用ViwlsEdge这种vio边的构造函数,这种vio边里储存着imu数据
        pose_graph::Edge* edge(
                new vi_map::ViwlsEdge(id, from, to, imu_timestamps, imu_data));
        this->addEdge(pose_graph::Edge::UniquePtr(edge));
    }

//当要合并节点时调用。merge_into_vertex_id是要合并的节点id，edge_between_vertices这条边连接着要合并的节点（最近关键帧）
// 和被合并的节点（普通帧），edge_after_next_vertex是普通帧的出边
    void PoseGraph::mergeNeighboringViwlsEdges(
            const pose_graph::VertexId& merge_into_vertex_id,
            const ViwlsEdge& edge_between_vertices,
            const ViwlsEdge& edge_after_next_vertex)
    {
        CHECK_EQ(merge_into_vertex_id, edge_between_vertices.from());
        CHECK_EQ(edge_between_vertices.to(), edge_after_next_vertex.from());//这个对应的节点都是普通帧所在的节点

        const Eigen::Matrix<int64_t, 1, Eigen::Dynamic>& imu_timestamps =//这两个节点间的imu时间戳
                edge_between_vertices.getImuTimestamps();
        const Eigen::Matrix<double, 6, Eigen::Dynamic>& imu_data =//这两个节点间的imu数据
                edge_between_vertices.getImuData();
        CHECK_EQ(imu_timestamps.cols(), imu_data.cols());//在这两个节点的时间戳之间，每接收一次imu数据，列就会加1

        const Eigen::Matrix<int64_t, 1, Eigen::Dynamic>& next_edge_imu_timestamps =//被合并的节点和它后一个节点间的imu时间戳
                edge_after_next_vertex.getImuTimestamps();
        const Eigen::Matrix<double, 6, Eigen::Dynamic>& next_edge_imu_data =//被合并的节点和它后一个节点间的imu数据
                edge_after_next_vertex.getImuData();
        CHECK_EQ(next_edge_imu_timestamps.cols(), next_edge_imu_data.cols());

        Eigen::Matrix<int64_t, 1, Eigen::Dynamic> new_imu_timestamps;
        Eigen::Matrix<double, 6, Eigen::Dynamic> new_imu_data;

        // We need to drop one measurement that was common for both edges.
        const int total_imu_measurements =
                imu_timestamps.cols() + next_edge_imu_timestamps.cols() - 1;
        new_imu_timestamps.resize(Eigen::NoChange, total_imu_measurements);
        new_imu_data.resize(Eigen::NoChange, total_imu_measurements);

        //因为要去掉这个普通帧的节点，所以需要把它前面的节点和后面的节点之间组成新的边，相当与两条边并一条，
        // 所以要将之前两条边的imu数据连接起来形成新的边的imu测量数据
        new_imu_timestamps << imu_timestamps.leftCols(imu_timestamps.cols() - 1),
                next_edge_imu_timestamps;
        new_imu_data << imu_data.leftCols(imu_data.cols() - 1), next_edge_imu_data;

        pose_graph::EdgeId new_edge_id;//产生一条新的边
        common::generateId(&new_edge_id);

        const pose_graph::VertexId new_edge_to_vertex = edge_after_next_vertex.to();//被合并的这个节点它的后一个节点

        // Delete old edges, create and add the new one.
        //去掉两条老边
        removeEdge(edge_between_vertices.id());
        removeEdge(edge_after_next_vertex.id());

        //添加一条新的边，从最近关键帧的节点指向被合并的节点的下一个节点
        addVIEdge(
                new_edge_id, merge_into_vertex_id, new_edge_to_vertex, new_imu_timestamps,
                new_imu_data);
        CHECK(edgeExists(new_edge_id));
    }

}  // namespace vi_map
