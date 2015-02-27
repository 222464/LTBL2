uniform sampler2D emissionTexture;

uniform vec2 targetSizeInv;

void main() {
	vec2 targetCoords = gl_FragCoord.xy * targetSizeInv;
	vec4 emissionColor = texture2D(emissionTexture, targetCoords);
	
    gl_FragColor = vec4(emissionColor.rgb, 1.0);
}