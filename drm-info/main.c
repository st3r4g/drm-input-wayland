#include <inttypes.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <xf86drmMode.h>

const char *conn_get_name(uint32_t type_id);

int main() {
	int fd = open("/dev/dri/card0", O_RDONLY);
	drmModeRes *res = drmModeGetResources(fd);
	for (int i=0; i<res->count_fbs; i++) {
		printf("Framebuffer %d\n", res->fbs[i]);
	}
	for (int i=0; i<res->count_crtcs; i++) {
		printf("Crtc %d\n", res->crtcs[i]);
	}
	for (int i=0; i<res->count_connectors; i++) {
		drmModeConnector *conn = drmModeGetConnector(fd, res->connectors[i]);
		printf("Connector %d (%s)", conn->connector_id,
		conn_get_name(conn->connector_type));
		if (conn->encoder_id) {
			drmModeEncoder *enc = drmModeGetEncoder(fd, conn->encoder_id);
			printf(" -> Crtc %d", enc->crtc_id);
			drmModeCrtc *crtc = drmModeGetCrtc(fd, enc->crtc_id);
			if (crtc->buffer_id) {
				printf(" -> Framebuffer %d\n", crtc->buffer_id);
			} else {
				printf(" -> NO FRAMEBUFFER\n");
			}
			drmModeFreeCrtc(crtc);
			drmModeFreeEncoder(enc);
		} else {
			printf(" -> NO CRTC\n");
		}
		drmModeFreeConnector(conn);
	}
	drmModeFreeResources(res);
	close(fd);
	return 0;
}

const char *conn_get_name(uint32_t type_id) {
	switch (type_id) {
	case DRM_MODE_CONNECTOR_Unknown:     return "Unknown";
	case DRM_MODE_CONNECTOR_VGA:         return "VGA";
	case DRM_MODE_CONNECTOR_DVII:        return "DVI-I";
	case DRM_MODE_CONNECTOR_DVID:        return "DVI-D";
	case DRM_MODE_CONNECTOR_DVIA:        return "DVI-A";
	case DRM_MODE_CONNECTOR_Composite:   return "Composite";
	case DRM_MODE_CONNECTOR_SVIDEO:      return "SVIDEO";
	case DRM_MODE_CONNECTOR_LVDS:        return "LVDS";
	case DRM_MODE_CONNECTOR_Component:   return "Component";
	case DRM_MODE_CONNECTOR_9PinDIN:     return "DIN";
	case DRM_MODE_CONNECTOR_DisplayPort: return "DP";
	case DRM_MODE_CONNECTOR_HDMIA:       return "HDMI-A";
	case DRM_MODE_CONNECTOR_HDMIB:       return "HDMI-B";
	case DRM_MODE_CONNECTOR_TV:          return "TV";
	case DRM_MODE_CONNECTOR_eDP:         return "eDP";
	case DRM_MODE_CONNECTOR_VIRTUAL:     return "Virtual";
	case DRM_MODE_CONNECTOR_DSI:         return "DSI";
	default:                             return "Unknown";
	}
}

