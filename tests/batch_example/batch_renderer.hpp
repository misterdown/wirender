#ifndef BATCH_RENDERER_HPP_
#define BATCH_RENDERER_HPP_ 1

#include <pickmelib/inplacevector.hpp>
#include <glm/glm.hpp>
#include <wiender.hpp>
#include <array>

struct batch_renderer {
    private:
    struct invertex {
        glm::vec2 pos;
        glm::vec2 uv;
        float textureId;
    };

    public:
    struct camera {
        glm::vec2 position;
        glm::vec2 scale;
    };
    struct vertex {
        glm::vec2 pos;
        vertex() : pos() {}
        vertex(const glm::vec2& pos) : pos(pos) {}
        vertex(float x, float y) : pos(x, y) {}
    };
    enum struct overflow_strategy {
        SKIP_PRIMITIVE,
        THROW_EXCEPTION,
        EXECUTE_AND_CLEAN,
    };
    struct triangle {
        std::array<vertex, 3> vertices;
        triangle() : vertices() {}
        triangle(const std::array<vertex, 3>& vertices) : vertices(vertices) {}
        triangle(const vertex& a, const vertex& b, const vertex& c) : vertices({a, b, c}) {}
    };
    struct quadrilateral {
        std::array<vertex, 4> vertices;
        quadrilateral() : vertices() {}
        quadrilateral(const std::array<vertex, 4>& vertices) : vertices(vertices) {}
        quadrilateral(const vertex& a, const vertex& b, const vertex& c, const vertex& d) : vertices({a, b, c, d}) {}
    };
    struct rectangle {
        glm::vec2 position;
        glm::vec2 rectExtent;
        float rotation;
        rectangle() : position(), rectExtent(), rotation(0.0f) {}
        rectangle(const glm::vec2& position, const glm::vec2& rectExtent = glm::vec2(1.0f, 1.0f), float rotation = 0.0f) : position(position), rectExtent(rectExtent), rotation(rotation) {}
    };

    wiender::wienderer* wienderer_;
    uint32_t maxVertices_;
    uint32_t maxIndices_;
    std::unique_ptr<wiender::buffer> vertexBuffer_;
    uint32_t currentVertex_;
    invertex* vertexData_;
    std::unique_ptr<wiender::buffer> indexBuffer_;
    uint32_t currentIndex_;
    uint32_t* indexData_;
    wcs::inplace_vector<const wiender::texture*, 8> textures_;
    std::unique_ptr<wiender::shader> batchShader_;
    overflow_strategy overflowStrategy_;
    camera cameraData_;
    camera* cameraUniformBuffer_;

    batch_renderer();
    batch_renderer(wiender::wienderer* wiender, uint32_t maxVertices, overflow_strategy overflowStrategy = overflow_strategy::SKIP_PRIMITIVE);
    batch_renderer(const batch_renderer&) = delete;
    batch_renderer(batch_renderer&&);
    batch_renderer& operator=(const batch_renderer&) = delete;
    batch_renderer& operator=(batch_renderer&&);

    int get_texture_index(const wiender::texture* texr);
    camera& get_camera() noexcept;
    const camera& get_camera() const noexcept;
    void add_triangle(const triangle& tri, const wiender::texture* texr);
    void add_quadrilateral(const quadrilateral& quad, const wiender::texture* texr);
    void add_rectangle(const rectangle& rect, const wiender::texture* texr, bool posIsCentre = false);
    void prepare_execute();
    void record_draw();
    void clean_batch();
};
#endif // BATCH_RENDERER_HPP_ 