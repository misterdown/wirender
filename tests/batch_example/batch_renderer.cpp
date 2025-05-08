#include <ifstream>

#include "batch_renderer.hpp"

using namespace wiender;
using vertex_input_attribute = shader::vertex_input_attribute;
using stage = shader::stage;
using primitive_topology = shader::primitive_topology;
using polygon_mode = shader::polygon_mode;
using cull_mode = shader::cull_mode;
using namespace glm;


std::vector<char> read_binary_file(const std::string& file_path) {
    std::ifstream in(file_path, std::ios::binary);
    in.seekg(0, std::ios::end);
    size_t sz = in.tellg();
    in.seekg(0);

    std::vector<unsigned char> v(sz);
    in.read((char*)v.data(), sz);

    return v;
}

#define __BATCH_RENDERER_BATCH_OVERFLOW \
    if (overflowStrategy_ == batch_renderer::overflow_strategy::SKIP_PRIMITIVE) { \
        return; \
    } else if (overflowStrategy_ == batch_renderer::overflow_strategy::EXECUTE_AND_CLEAN) { \
        prepare_execute(); \
        wienderer_->execute(); \
        clean_batch(); \
    } else if (overflowStrategy_ == batch_renderer::overflow_strategy::THROW_EXCEPTION) { \
        throw std::runtime_error("batch_renderer::batch overflow"); \
    }


batch_renderer::batch_renderer() = default;
batch_renderer::batch_renderer(wienderer* wrer, uint32_t maxVertices, overflow_strategy overflowStrategy) :
        wienderer_(wrer),
        maxVertices_(maxVertices), maxIndices_(maxVertices * 6),
        vertexBuffer_(), currentVertex_(0), vertexData_(nullptr),
        indexBuffer_(), currentIndex_(0), indexData_(nullptr),
        overflowStrategy_(overflowStrategy),
        cameraData_(), cameraUniformBuffer_(nullptr) {
    if (wienderer_ == nullptr) {
        throw std::runtime_error("failed to create batch renderer");
    }
    vertexBuffer_ = wienderer_->create_buffer(buffer::type::CPU_SIDE_VERTEX, maxVertices_ * sizeof(invertex));
    indexBuffer_ = wienderer_->create_buffer(buffer::type::CPU_SIDE_INDEX, maxIndices_ * sizeof(uint32_t));
    batchShader_ = wienderer_->create_shader(
        shader::create_info(
            std::vector<stage> {
                stage(stage::kind::VERTEX, read_binary_file(    "../assets/multi_texturev.spirv")),
                stage(stage::kind::FRAGMENT, read_binary_file(  "../assets/multi_texturef.spirv"))
            },
            std::vector<vertex_input_attribute>{
                vertex_input_attribute(vertex_input_attribute::format::FLOAT_VEC2, 0, 0, 0),
                vertex_input_attribute(vertex_input_attribute::format::FLOAT_VEC2, 1, offsetof(invertex, uv), 0),
                vertex_input_attribute(vertex_input_attribute::format::FLOAT_SCALAR, 2, offsetof(invertex, textureId), 0)
            },
            primitive_topology::TRIANGLES_LIST,
            polygon_mode::FILL,
            cull_mode::NONE,
            true,
            true
        ));
    cameraUniformBuffer_ = (camera*)batchShader_->get_uniform_buffer_info(0).data;
    cameraData_.position = vec2();
    cameraData_.scale = vec2(0.1f, 0.1f);
    vertexData_ = (invertex*)vertexBuffer_->map();
    indexData_ = (uint32_t*)indexBuffer_->map();
}
batch_renderer::batch_renderer(batch_renderer&& other)  :
        wienderer_(other.wienderer_),
        maxVertices_(other.maxVertices_), maxIndices_(other.maxIndices_),
        vertexBuffer_(std::move(other.vertexBuffer_)), currentVertex_(other.currentVertex_), vertexData_(other.vertexData_),
        indexBuffer_(std::move(other.indexBuffer_)), currentIndex_(other.currentIndex_), indexData_(other.indexData_),
        textures_(std::move(other.textures_)),
        overflowStrategy_(other.overflowStrategy_),
        cameraData_(other.cameraData_), cameraUniformBuffer_(other.cameraUniformBuffer_) {
    
}
batch_renderer& batch_renderer::operator=(batch_renderer&&) = default;

int batch_renderer::get_texture_index(const wiender::texture* texr) {
    int textureIndex = -1;
    for (int i = 0; i < textures_.size(); ++i) {
        if (textures_[i] == texr) {
            textureIndex = i;
            break;
        }
    }
    if (textureIndex == -1) {
        try {
            textureIndex = textures_.size();
            textures_.emplace_back(texr);
        } catch(...) {
            // nothing here
        }
    }
    return textureIndex;
}

batch_renderer::camera& batch_renderer::get_camera() noexcept {
    return cameraData_;
}
const batch_renderer::camera& batch_renderer::get_camera() const noexcept {
    return cameraData_;
}
void batch_renderer::add_triangle(const batch_renderer::triangle& tri, const wiender::texture* tetr) {
    if (currentIndex_ + 3 > maxIndices_) {
        __BATCH_RENDERER_BATCH_OVERFLOW
    }
    if (currentVertex_ + 3 > maxVertices_) {
        __BATCH_RENDERER_BATCH_OVERFLOW
    }
    const float textureIndex = (float)get_texture_index(tetr);
    if (textureIndex < 0.0f) {
        __BATCH_RENDERER_BATCH_OVERFLOW
    }
    vertexData_[currentVertex_] =       invertex{ tri.vertices[0].pos, vec2(0.0f, 0.0f), textureIndex};
    vertexData_[currentVertex_ + 1] =   invertex{ tri.vertices[1].pos, vec2(1.0f, 0.0f), textureIndex};
    vertexData_[currentVertex_ + 2] =   invertex{ tri.vertices[2].pos, vec2(0.0f, 1.0f), textureIndex};
    indexData_[currentIndex_] =         currentVertex_;
    indexData_[currentIndex_ + 1] =     currentVertex_ + 1;
    indexData_[currentIndex_ + 2] =     currentVertex_ + 2;
    currentVertex_ += 3;
    currentIndex_ += 3;
}
void batch_renderer::add_quadrilateral(const batch_renderer::quadrilateral& qua, const wiender::texture* tetr) {
    if (currentIndex_ + 6 > maxIndices_) {
        __BATCH_RENDERER_BATCH_OVERFLOW
    }
    if (currentVertex_ + 4 > maxVertices_) {
        __BATCH_RENDERER_BATCH_OVERFLOW
    }
    const float textureIndex = (float)get_texture_index(tetr);
    if (textureIndex < 0.0f) {
        __BATCH_RENDERER_BATCH_OVERFLOW
    }

    vertexData_[currentVertex_] =       invertex{ qua.vertices[0].pos, vec2(0.0f, 0.0f), textureIndex};
    vertexData_[currentVertex_ + 1] =   invertex{ qua.vertices[1].pos, vec2(1.0f, 0.0f), textureIndex};
    vertexData_[currentVertex_ + 2] =   invertex{ qua.vertices[2].pos, vec2(0.0f, 1.0f), textureIndex};
    vertexData_[currentVertex_ + 3] =   invertex{ qua.vertices[3].pos, vec2(1.0f, 1.0f), textureIndex};
    indexData_[currentIndex_] =         currentVertex_;
    indexData_[currentIndex_ + 1] =     currentVertex_ + 1;
    indexData_[currentIndex_ + 2] =     currentVertex_ + 2;
    indexData_[currentIndex_ + 3] =     currentVertex_ + 1;
    indexData_[currentIndex_ + 4] =     currentVertex_ + 2;
    indexData_[currentIndex_ + 5] =     currentVertex_ + 3;
    currentVertex_ += 4;
    currentIndex_ += 6;
}
void batch_renderer::add_rectangle(const batch_renderer::rectangle& rect, const wiender::texture* tetr, bool posIsCentre) {
    // Calculate the half extents
    const glm::vec2 minE = posIsCentre ? rect.rectExtent * -0.5f : vec2(0.0f);
    const glm::vec2 maxE = posIsCentre ? rect.rectExtent * 0.5f : rect.rectExtent;

    // Calculate the four corners of the rectangle in local space
    glm::vec2 topLeft(minE.x, maxE.y);
    glm::vec2 topRight(maxE.x, maxE.y);
    glm::vec2 bottomLeft(minE.x, minE.y);
    glm::vec2 bottomRight(maxE.x, minE.y);

    if (rect.rotation != 0.0f) {
        // Create a 2x2 rotation matrix
        const float cosTheta = cos(rect.rotation);
        const float sinTheta = sin(rect.rotation);
        const glm::mat2 rotationMatrix(cosTheta, -sinTheta, sinTheta, cosTheta);

        // Rotate the corners
        topLeft = rotationMatrix * topLeft;
        topRight = rotationMatrix * topRight;
        bottomLeft = rotationMatrix * bottomLeft;
        bottomRight = rotationMatrix * bottomRight;
    }

    // Translate the corners to the rectangle's position
    topLeft += rect.position;
    topRight += rect.position;
    bottomLeft += rect.position;
    bottomRight += rect.position;

    // Add the quadrilateral to the batch renderer
    quadrilateral qua(
        {
            topLeft,
            topRight,
            bottomLeft,
            bottomRight
        }
    );

    add_quadrilateral(qua, tetr);
}


void batch_renderer::prepare_execute() {
    vertexBuffer_->update_data();
    indexBuffer_->update_data();
    for (uint32_t i = 0; i < textures_.size(); ++i) {
        batchShader_->bind_texture(1, i, textures_[i]);
    }
    cameraUniformBuffer_->position = -cameraData_.position;
    cameraUniformBuffer_->scale = cameraData_.scale;
}
void batch_renderer::record_draw() {
    vertexBuffer_->bind();
    indexBuffer_->bind();
    batchShader_->set();
    wienderer_->begin_render();
    wienderer_->draw_indexed(maxIndices_, 0, 1);
    wienderer_->end_render();
}
void batch_renderer::clean_batch() {
 // memset(vertexData_, 0, currentVertex_ * sizeof(invertex)); // probably, could be removed
    memset(indexData_, 0, currentIndex_ * sizeof(uint32_t)); 
    currentIndex_ = 0;
    currentVertex_ = 0;
    textures_.clear();
}
