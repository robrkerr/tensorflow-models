package(default_visibility = ["//visibility:public"])

cc_library(
    name = "generator_component",
    srcs = ["generator_component.cc"],
    hdrs = ["generator_component.h"],
    deps = [
        ":generator_link_feature_extractor",
        ":generator_transition_state",
        "//dragnn/components/util:bulk_feature_extractor",
        "//dragnn/core:beam",
        "//dragnn/core:component_registry",
        "//dragnn/core:input_batch_cache",
        "//dragnn/core/interfaces:component",
        "//dragnn/core/interfaces:transition_state",
        "//dragnn/io:sentence_input_batch",
        "//dragnn/io:syntaxnet_sentence",
        "//dragnn/protos:data_proto",
        "//dragnn/protos:spec_proto",
        "//dragnn/protos:trace_proto",
        "//syntaxnet:base",
        "//syntaxnet:generator_transitions",
        "//syntaxnet:registry",
        "//syntaxnet:sparse_proto",
        "//syntaxnet:task_context",
        "//syntaxnet:task_spec_proto",
        "//syntaxnet:utils",
        "@org_tensorflow//tensorflow/core:lib",  # For tf/core/platform/logging.h
    ],
    alwayslink = 1,
)

cc_library(
    name = "generator_link_feature_extractor",
    srcs = ["generator_link_feature_extractor.cc"],
    hdrs = ["generator_link_feature_extractor.h"],
    deps = [
        "//dragnn/protos:spec_proto",
        "//syntaxnet:embedding_feature_extractor",
        "//syntaxnet:generator_transitions",
        "//syntaxnet:task_context",
        "@org_tensorflow//tensorflow/core:lib",  # For tf/core/platform/logging.h
    ],
)

cc_library(
    name = "generator_transition_state",
    srcs = ["generator_transition_state.cc"],
    hdrs = ["generator_transition_state.h"],
    deps = [
        "//dragnn/core/interfaces:cloneable_transition_state",
        "//dragnn/core/interfaces:transition_state",
        "//dragnn/io:syntaxnet_sentence",
        "//dragnn/protos:trace_proto",
        "//syntaxnet:base",
        "//syntaxnet:generator_transitions",
        "@org_tensorflow//tensorflow/core:lib",  # For tf/core/platform/logging.h
    ],
)
