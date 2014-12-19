uniform sampler2D emissionTexture;
uniform sampler2D penumbraTexture;

uniform float lightBrightness;
uniform float darkBrightness;

uniform vec2 targetSizeInv;

void main() {
	vec3 emission = texture2D(emissionTexture, gl_FragCoord.xy * targetSizeInv).rgb;

    float penumbra = texture2D(penumbraTexture, gl_TexCoord[0].xy).x;
	
	float shadow = (lightBrightness - darkBrightness) * penumbra + darkBrightness;

    gl_FragColor = vec4(emission, 1.0 - shadow);
}