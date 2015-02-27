#include <ltbl2/lighting/LightPointEmission.h>
#include <ltbl2/lighting/LightShape.h>
#include <ltbl2/lighting/LightSystem.h>

#include <assert.h>

using namespace ltbl;

void LightPointEmission::render(const sf::View &view, sf::RenderTexture &lightTempTexture, sf::RenderTexture &emissionTempTexture, const std::vector<QuadtreeOccupant*> &shapes, sf::Shader &unshadowShader, sf::Shader &lightOverShapeShader) {
    LightSystem::clear(emissionTempTexture, sf::Color::Black);

    emissionTempTexture.setView(view);

    emissionTempTexture.draw(_emissionSprite);

    emissionTempTexture.display();
    
    LightSystem::clear(lightTempTexture, sf::Color(0, 0, 0, 0));

    lightTempTexture.setView(view);

    lightTempTexture.draw(_emissionSprite);

    sf::Vector2f castCenter = _emissionSprite.getTransform().transformPoint(_localCastCenter);
    
    float shadowExtension = _shadowOverExtendMultiplier * (getAABB().width + getAABB().height);

    std::vector<LightSystem::Penumbra> penumbras;

    struct OuterEdges {
        std::vector<int> _outerBoundaryIndices;
        std::vector<sf::Vector2f> _outerBoundaryVectors;
    };

    std::vector<OuterEdges> outerEdges(shapes.size());

    // Mask off light shape (over-masking - mask too much, reveal penumbra/antumbra afterwards)
    for (int i = 0; i < shapes.size(); i++) {
        LightShape* pLightShape = static_cast<LightShape*>(shapes[i]);

        // Get boundaries
        std::vector<int> innerBoundaryIndices;
        std::vector<sf::Vector2f> innerBoundaryVectors;

        LightSystem::getPenumbrasPoint(penumbras, innerBoundaryIndices, innerBoundaryVectors, outerEdges[i]._outerBoundaryIndices, outerEdges[i]._outerBoundaryVectors, pLightShape->_shape, castCenter, _sourceRadius);

        if (innerBoundaryIndices.size() != 2)
            continue;

        // Render shape
        if (!pLightShape->_renderLightOverShape) {
            pLightShape->_shape.setFillColor(sf::Color(0, 0, 0, 0));

            lightTempTexture.draw(pLightShape->_shape);
        }

        sf::RenderStates maskRenderStates;
        maskRenderStates.blendMode = sf::BlendNone;

        sf::ConvexShape maskShape;

        maskShape.setPointCount(4);

        maskShape.setPoint(0, pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(innerBoundaryIndices[0])));
        maskShape.setPoint(1, pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(innerBoundaryIndices[1])));
        maskShape.setPoint(2, pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(innerBoundaryIndices[1])) + vectorNormalize(innerBoundaryVectors[1]) * shadowExtension);
        maskShape.setPoint(3, pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(innerBoundaryIndices[0])) + vectorNormalize(innerBoundaryVectors[0]) * shadowExtension);

        maskShape.setFillColor(sf::Color(0, 0, 0, 0));

        lightTempTexture.draw(maskShape, maskRenderStates);
    }

    sf::VertexArray vertexArray;

    vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);

    vertexArray.resize(3);

    sf::RenderStates penumbraRenderStates;
    penumbraRenderStates.blendMode = sf::BlendMode(sf::BlendMode::OneMinusDstAlpha, sf::BlendMode::DstAlpha, sf::BlendMode::Equation::Add, sf::BlendMode::One, sf::BlendMode::One, sf::BlendMode::Equation::Add);
    penumbraRenderStates.shader = &unshadowShader;

    {
        // Unmask with penumbras
        for (int j = 0; j < penumbras.size(); j++) {
            unshadowShader.setParameter("lightBrightness", penumbras[j]._lightBrightness);
            unshadowShader.setParameter("darkBrightness", penumbras[j]._darkBrightness);

            vertexArray[0].position = penumbras[j]._source;
            vertexArray[1].position = penumbras[j]._source + vectorNormalize(penumbras[j]._lightEdge) * shadowExtension;
            vertexArray[2].position = penumbras[j]._source + vectorNormalize(penumbras[j]._darkEdge) * shadowExtension;

            vertexArray[0].texCoords = sf::Vector2f(0.0f, 1.0f);
            vertexArray[1].texCoords = sf::Vector2f(1.0f, 0.0f);
            vertexArray[2].texCoords = sf::Vector2f(0.0f, 0.0f);

            lightTempTexture.draw(vertexArray, penumbraRenderStates);
        }
    }

    for (int i = 0; i < shapes.size(); i++) {
        LightShape* pLightShape = static_cast<LightShape*>(shapes[i]);

        sf::RenderStates maskRenderStates;
        maskRenderStates.blendMode = sf::BlendNone;

        if (outerEdges[i]._outerBoundaryIndices.size() != 2)
            continue;

        sf::Vector2f as = pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(outerEdges[i]._outerBoundaryIndices[0]));
        sf::Vector2f bs = pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(outerEdges[i]._outerBoundaryIndices[1]));
        sf::Vector2f ad = outerEdges[i]._outerBoundaryVectors[0];
        sf::Vector2f bd = outerEdges[i]._outerBoundaryVectors[1];

        sf::Vector2f intersection;

        if (rayIntersect(as, ad, bs, bd, intersection)) {
            sf::ConvexShape maskShape;

            maskShape.setPointCount(3);

            maskShape.setPoint(0, as);
            maskShape.setPoint(1, bs);
            maskShape.setPoint(2, intersection);
    
            maskShape.setFillColor(sf::Color::Black);

            lightTempTexture.draw(maskShape);
        }
        else {
            sf::ConvexShape maskShape;

            maskShape.setPointCount(4);

            maskShape.setPoint(0, as);
            maskShape.setPoint(1, bs);
            maskShape.setPoint(2, bs + vectorNormalize(bd) * shadowExtension);
            maskShape.setPoint(3, as + vectorNormalize(ad) * shadowExtension);

            maskShape.setFillColor(sf::Color::Black);

            lightTempTexture.draw(maskShape);
        }
    }

    for (int i = 0; i < shapes.size(); i++) {
        LightShape* pLightShape = static_cast<LightShape*>(shapes[i]);

        if (pLightShape->_renderLightOverShape) {
            pLightShape->_shape.setFillColor(sf::Color::White);

            lightTempTexture.draw(pLightShape->_shape, &lightOverShapeShader);
        }
        else {
            pLightShape->_shape.setFillColor(sf::Color::Black);

            lightTempTexture.draw(pLightShape->_shape);
        }
    }

    lightTempTexture.display();
}