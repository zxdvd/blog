```metadata
tags: database, design, workflow
```

## workflow engine design

A company may have many kinds of workflows, like vavaction, expenses. A workflow engine
 can be used so that you needn't to write each workflow with hard code.

###A general design

Each workflow has a name, like `request for vacation`.

```
table: workflow
columns:
    - id              int
    - name            text
```

A workflow will have multiple nodes. Each node needs a role or a specific person to approve.
`seq_num` is sequence of the node in the workflow. You can get all nodes of a workflow with
 order via `select * from workflow_node order by seq_num`.

```
table: workflow_node
columns:
    - id              int
    - workflow_id     int
    - role_id         int
    - staff_id        int
    - seq_num         int
```

The workflow request table records all request of each workflow. `current_node` is the
 current node of the request. And we also cache a `current_state` here.

```
table: workflow_request
columns:
    - id                        int
    - workflow_id               int
    - creator_id                int
    - current_node_id           int
    - current_state             int
    - created_at                timestamptz
```

The action records all actions to the request, like approve or reject.

```
table: workflow_request_action
columns:
    - id                         int
    - workflow_request_id        int
    - creator_id                 int
    - state                      int
    - note                       int
    - created_at                timestamptz
```

### extending
Above is very simple and basic workflow engine design. I've thought about following
 questions.

- each request may have association data of specific business
- some request or workflow node may have duration time or deadline requirement
- some may want to deduplicate repeated persons in the workflow
- you may want to create workflow with dynamic nodes, for example, a request needs to be
  aproved by all ancestral managers till CEO
