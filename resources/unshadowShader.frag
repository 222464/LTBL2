uniform sampler2D penumbraTexture;

uniform float lightBrightness;
uniform float darkBrightness;

void main() {
    float penumbra = texture2D(penumbraTexture, gl_TexCoord[0].xy).x;
	
	float shadow = (lightBrightness - darkBrightness) * penumbra + darkBrightness;

    gl_FragColor = vec4(vec3(1.0 - shadow), 1.0);
}